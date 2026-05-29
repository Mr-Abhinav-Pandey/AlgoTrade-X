#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <exception>
#include <iomanip>
#include <chrono>
#include "market_snapshot.h"
#include "variantforge_engine.cpp"
using namespace std;

// Forward declarations (fetcher.cpp and parser.cpp)
string fetch_binance(const string& endpoint);
double parse_double(const string& json, const string& key);
string parse_string(const string& json, const string& key);


struct FenwickTree {
    vector<int> bit;
    int n;
    FenwickTree(int n) : n(n), bit(n + 1, 0) {}
    void update(int idx, int delta) {
        for (; idx <= n; idx += idx & -idx) bit[idx] += delta;
    }
    int prefix_sum(int idx) {
        int s = 0;
        for (; idx > 0; idx -= idx & -idx) s += bit[idx];
        return s;
    }
    int query(int L, int R) {
        if (L > R) return 0;
        return prefix_sum(R) - prefix_sum(L - 1);
    }
};

struct PriceGrid {
    double base_price, tick;
    int buckets;
    PriceGrid(double base, double tick_size, int num_buckets)
        : base_price(base), tick(tick_size), buckets(num_buckets) {}
    int to_index(double price) {
        int idx = (int)((price - base_price) / tick) + 1;
        if (idx < 1) idx = 1;
        if (idx > buckets) idx = buckets;
        return idx;
    }
};

struct OrderBook {
    FenwickTree bids, asks;
    PriceGrid grid;
    double mid_price;
    OrderBook(double base, double tick, int buckets)
        : bids(buckets), asks(buckets), grid(base, tick, buckets), mid_price(0) {}
    void add_bid(double price, double qty) {
        int units = (int)(qty * 100);
        if (units > 0) bids.update(grid.to_index(price), units);
    }
    void add_ask(double price, double qty) {
        int units = (int)(qty * 100);
        if (units > 0) asks.update(grid.to_index(price), units);
    }
    double bid_volume(double L, double R) {
        return bids.query(grid.to_index(L), grid.to_index(R)) / 100.0;
    }
    double ask_volume(double L, double R) {
        return asks.query(grid.to_index(L), grid.to_index(R)) / 100.0;
    }
};

vector<pair<double,double>> parse_orders(const string& json, const string& side) {
    vector<pair<double,double>> orders;
    string key = "\"" + side + "\"";
    size_t pos = json.find(key);
    if (pos == string::npos) return orders;
    size_t arr_start = json.find('[', pos);
    int depth = 0;
    size_t arr_end = arr_start;
    for (size_t i = arr_start; i < json.size(); ++i) {
        if (json[i] == '[') depth++;
        else if (json[i] == ']') { depth--; if (depth == 0) { arr_end = i; break; } }
    }
    string arr = json.substr(arr_start, arr_end - arr_start + 1);
    size_t p = 0;
    while ((p = arr.find('[', p + 1)) != string::npos) {
        size_t q = arr.find(']', p);
        string pair_str = arr.substr(p + 1, q - p - 1);
        size_t q1s = pair_str.find('"') + 1;
        size_t q1e = pair_str.find('"', q1s);
        size_t q2s = pair_str.find('"', q1e + 1) + 1;
        size_t q2e = pair_str.find('"', q2s);
        double price = stod(pair_str.substr(q1s, q1e - q1s));
        double qty   = stod(pair_str.substr(q2s, q2e - q2s));
        orders.push_back({price, qty});
        p = q;
    }
    return orders;
}

OrderBook load_order_book_from_snapshot(const MarketSnapshot& snap) {
    OrderBook book(snap.btc_mid - 500.0, 1.0, 1000);
    book.mid_price = snap.btc_mid;
    for (const auto& b : snap.bids) book.add_bid(b.first, b.second);
    for (const auto& a : snap.asks) book.add_ask(a.first, a.second);
    return book;
}

static bool quote_valid(double bid, double ask) {
    return bid > 0 && ask > 0 && bid < ask;
}

static bool parse_book_ticker_bid_ask(const string& json, double& bid, double& ask) {
    if (json.empty()) return false;
    try {
        bid = parse_double(json, "bidPrice");
        ask = parse_double(json, "askPrice");
    } catch (const exception&) {
        return false;
    }
    return quote_valid(bid, ask);
}

static bool btc_bid_ask_from_depth(const MarketSnapshot& snap, double& bid, double& ask) {
    if (snap.bids.empty() || snap.asks.empty()) return false;
    bid = 0;
    ask = 1e300;
    for (const auto& level : snap.bids) bid = max(bid, level.first);
    for (const auto& level : snap.asks) ask = min(ask, level.first);
    return quote_valid(bid, ask);
}

// Best-effort bid/ask; never fails the snapshot build. Sets executable_ready when all pairs OK.
static bool try_fill_executable_quotes(MarketSnapshot& snap) {
    snap.executable_ready = false;

    if (!btc_bid_ask_from_depth(snap, snap.btcusdt_bid, snap.btcusdt_ask)) {
        cerr << "[WARN] Executable layer: BTCUSDT bid/ask unavailable (depth)\n";
        return false;
    }

    struct PairFetch { const char* symbol; double* bid; double* ask; };
    PairFetch pairs[] = {
        {"ETHUSDT", &snap.ethusdt_bid, &snap.ethusdt_ask},
        {"BNBUSDT", &snap.bnbusdt_bid, &snap.bnbusdt_ask},
        {"ETHBTC",  &snap.ethbtc_bid,  &snap.ethbtc_ask},
        {"BNBBTC",  &snap.bnbbtc_bid,  &snap.bnbbtc_ask},
    };

    for (const PairFetch& p : pairs) {
        string url = string("https://api.binance.com/api/v3/ticker/bookTicker?symbol=") + p.symbol;
        string json = fetch_binance(url);
        if (!parse_book_ticker_bid_ask(json, *p.bid, *p.ask)) {
            cerr << "[WARN] Executable layer: " << p.symbol << " bid/ask unavailable\n";
            return false;
        }
    }

    snap.executable_ready = true;
    return true;
}

// Fetches all REST data for one live scan into a single snapshot.
static bool build_market_snapshot_rest(MarketSnapshot& snap) {
    snap = MarketSnapshot{};
    snap.assembled_ms = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()).count();

    string price_json = fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT");
    if (price_json.empty()) {
        cerr << "[ERROR] Price fetch failed\n";
        return false;
    }
    try {
        snap.btc_mid = parse_double(price_json, "price");
        snap.btcusdt = snap.btc_mid;
    } catch (const exception& ex) {
        cerr << "[ERROR] Price parse failed: " << ex.what() << "\n";
        return false;
    }

    string book_json = fetch_binance("https://api.binance.com/api/v3/depth?symbol=BTCUSDT&limit=20");
    if (book_json.empty()) {
        cerr << "[ERROR] Depth fetch failed\n";
        return false;
    }
    snap.bids = parse_orders(book_json, "bids");
    snap.asks = parse_orders(book_json, "asks");
    if (snap.bids.empty() || snap.asks.empty()) {
        cerr << "[ERROR] Depth parse failed (empty book)\n";
        return false;
    }

    try {
        snap.ethusdt = parse_double(
            fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=ETHUSDT"), "price");
        snap.bnbusdt = parse_double(
            fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BNBUSDT"), "price");
        snap.ethbtc = parse_double(
            fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=ETHBTC"), "price");
        snap.bnbbtc = parse_double(
            fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BNBBTC"), "price");
    } catch (const exception& ex) {
        cerr << "[ERROR] Cross-rate fetch/parse failed: " << ex.what() << "\n";
        return false;
    }

    if (!snap.is_complete()) {
        cerr << "[ERROR] Incomplete market snapshot\n";
        return false;
    }

    snap.executable_ready = try_fill_executable_quotes(snap);
    return true;
}

static constexpr double TAKER_FEE = 0.001;

Edge make_rate_edge(int u, int v, double rate) {
    Edge e; e.u = u; e.v = v;
    rate = max(rate, 1e-12);
    e.w = (int)(-log(rate) * 10000);
    return e;
}

// Explicit triangular cycles (USDT=0, BTC=1, ETH=2, BNB=3) for auditable profit reporting.
static double best_triangular_gain(double btcusdt, double ethusdt, double bnbusdt,
                                   double ethbtc, double bnbbtc) {
    const double kEps = 1e-9;
    auto safe_mul = [kEps](double a, double b) {
        if (a <= kEps || b <= kEps) return 0.0;
        return a * b;
    };
    double best = 1.0;
    // USDT -> BTC -> ETH -> USDT
    best = max(best, safe_mul(safe_mul(1.0 / btcusdt, 1.0 / ethbtc), ethusdt));
    // USDT -> BTC -> BNB -> USDT
    best = max(best, safe_mul(safe_mul(1.0 / btcusdt, 1.0 / bnbbtc), bnbusdt));
    return best;
}

static double best_triangular_gain_executable(const MarketSnapshot& snap) {
    const double kEps = 1e-9;
    auto safe_mul = [kEps](double a, double b) {
        if (a <= kEps || b <= kEps) return 0.0;
        return a * b;
    };
    const double fee_scale = (1.0 - TAKER_FEE) * (1.0 - TAKER_FEE) * (1.0 - TAKER_FEE);

    double best = 1.0;
    // USDT -> BTC -> ETH -> USDT (buy @ ask, sell @ bid)
    best = max(best, safe_mul(
        safe_mul(1.0 / snap.btcusdt_ask, 1.0 / snap.ethbtc_ask), snap.ethusdt_bid));
    // USDT -> BTC -> BNB -> USDT
    best = max(best, safe_mul(
        safe_mul(1.0 / snap.btcusdt_ask, 1.0 / snap.bnbbtc_ask), snap.bnbusdt_bid));
    return best * fee_scale;
}

static vector<Edge> build_fx_edges_mid(const MarketSnapshot& snap) {
    vector<Edge> edges;
    const double btcusdt = snap.btcusdt;
    const double ethusdt = snap.ethusdt;
    const double bnbusdt = snap.bnbusdt;
    const double ethbtc = snap.ethbtc;
    const double bnbbtc = snap.bnbbtc;
    edges.push_back(make_rate_edge(0, 1, 1.0 / btcusdt));
    edges.push_back(make_rate_edge(1, 0, btcusdt));
    edges.push_back(make_rate_edge(0, 2, 1.0 / ethusdt));
    edges.push_back(make_rate_edge(2, 0, ethusdt));
    edges.push_back(make_rate_edge(0, 3, 1.0 / bnbusdt));
    edges.push_back(make_rate_edge(3, 0, bnbusdt));
    edges.push_back(make_rate_edge(1, 2, 1.0 / ethbtc));
    edges.push_back(make_rate_edge(2, 1, ethbtc));
    edges.push_back(make_rate_edge(1, 3, 1.0 / bnbbtc));
    edges.push_back(make_rate_edge(3, 1, bnbbtc));
    return edges;
}

static vector<Edge> build_fx_edges_executable(const MarketSnapshot& snap) {
    vector<Edge> edges;
    edges.push_back(make_rate_edge(0, 1, 1.0 / snap.btcusdt_ask));
    edges.push_back(make_rate_edge(1, 0, snap.btcusdt_bid));
    edges.push_back(make_rate_edge(0, 2, 1.0 / snap.ethusdt_ask));
    edges.push_back(make_rate_edge(2, 0, snap.ethusdt_bid));
    edges.push_back(make_rate_edge(0, 3, 1.0 / snap.bnbusdt_ask));
    edges.push_back(make_rate_edge(3, 0, snap.bnbusdt_bid));
    edges.push_back(make_rate_edge(1, 2, 1.0 / snap.ethbtc_ask));
    edges.push_back(make_rate_edge(2, 1, snap.ethbtc_bid));
    edges.push_back(make_rate_edge(1, 3, 1.0 / snap.bnbbtc_ask));
    edges.push_back(make_rate_edge(3, 1, snap.bnbbtc_bid));
    return edges;
}

void run_live_scan(int cycle, const MarketSnapshot& snap) {
    (void)cycle;
    const double current_price = snap.btc_mid;
    OrderBook book = load_order_book_from_snapshot(snap);

    double lo = current_price - 100, hi = current_price + 100;
    double bid_vol = book.bid_volume(lo, current_price);
    double ask_vol = book.ask_volume(current_price, hi);

    const int grid_buckets = 1000;
    const int live_N = grid_buckets;
    const int live_Q = max((int)(snap.bids.size() + snap.asks.size()), 1);

    const int exec_T = 20;
    const int exec_I = min(max((int)(bid_vol * 10.0 + 0.5), 1), exec_T);

    // Arbitrage scan (build graph before profiling problem B)
    cout << "\n--- ARBITRAGE SCAN ---\n";
    const int live_V = 4;
    vector<Edge> arb_graph = dedupe_fx_edges(build_fx_edges_mid(snap));
    const int live_E = (int)arb_graph.size();

    ProblemProfileA liveA = { live_N, live_Q, 0.5 };
    ProblemProfileB liveB = { live_V, live_E };
    ProblemProfileC liveC = { exec_T, exec_I };

    string algoA, algoB, algoC, logicA, logicB, logicC, tleA, tleB, tleC;
    algoA = DecisionEngine::select_algorithm_A(liveA, logicA, tleA);
    algoB = DecisionEngine::select_algorithm_B(liveB, logicB, tleB);
    algoC = DecisionEngine::select_algorithm_C(liveC, logicC, tleC);

    cout << "\n--- DECISION ENGINE ---\n";
    cout << "BTC Price : $" << current_price << "\n";
    cout << "Profile A : N=" << live_N << " Q=" << live_Q << " (bucket grid + book levels/scan)\n";
    cout << "Profile B : V=" << live_V << " E=" << live_E << " (deduped FX edges)\n";
    cout << "Profile C : T=" << exec_T << " I=" << exec_I << " (inventory capped by horizon)\n";
    cout << "Order Book : " << algoA << "  [" << (algoA.find("Fenwick") != string::npos ? "using Fenwick OrderBook" : "see stress mode") << "]\n";
    cout << "Arbitrage  : " << algoB << "\n";
    cout << "Execution  : " << algoC << "\n";

    bool graph_cycle_mid = arbitrage_negative_cycle(live_V, arb_graph);
    double tri_mid = best_triangular_gain(
        snap.btcusdt, snap.ethusdt, snap.bnbusdt, snap.ethbtc, snap.bnbbtc);
    const double fee_buffer = 1.0005;
    bool theoretical_edge = tri_mid > fee_buffer;

    cout << "Detector   : log-rate Bellman-Ford (super-source), edges=" << live_E << "\n";
    cout << "Theoretical (mid ticker):\n";
    cout << "  Neg-cycle  : " << (graph_cycle_mid ? "yes" : "no")
         << "  (stress path uses " << algoB << ")\n";
    cout << "  Best tri   : " << fixed << setprecision(6) << tri_mid
         << "x (USDT->BTC->alt->USDT)\n";
    cout << "  Mid edge   : " << (theoretical_edge ? "YES (tri > fee buffer)" : "NO") << "\n";
    cout.unsetf(ios::floatfield);

    bool tradable = false;
    double tri_exec = 1.0;
    bool graph_cycle_exec = false;

    if (!snap.executable_ready) {
        cout << "Executable (bid/ask + 3x0.1% fee):\n";
        cout << "  Status     : UNAVAILABLE (missing or invalid bid/ask; see [WARN] above)\n";
    } else {
        vector<Edge> arb_exec = dedupe_fx_edges(build_fx_edges_executable(snap));
        graph_cycle_exec = arbitrage_negative_cycle(live_V, arb_exec);
        tri_exec = best_triangular_gain_executable(snap);
        tradable = tri_exec > 1.0;

        cout << "Executable (bid/ask + 3x0.1% fee):\n";
        cout << "  Neg-cycle  : " << (graph_cycle_exec ? "yes" : "no") << "\n";
        cout << "  Best tri   : " << fixed << setprecision(6) << tri_exec
             << "x (USDT->BTC->alt->USDT)\n";
        cout.unsetf(ios::floatfield);
        double spread_bps = 10000.0 * (snap.btcusdt_ask - snap.btcusdt_bid) / snap.btc_mid;
        cout << "  BTC spread : " << fixed << setprecision(2) << spread_bps << " bps (depth top)\n";
        cout.unsetf(ios::floatfield);
    }

    cout << "Tradable     : " << (tradable ? "YES" : "NO") << " (executable triangle > 1.0)\n";

    if (snap.executable_ready) {
        bool mid_suggests = graph_cycle_mid || tri_mid > 1.0;
        if (mid_suggests && !tradable) {
            cout << "Explain      : theoretical edge ~" << fixed << setprecision(2)
                 << (tri_mid - 1.0) * 10000.0 << " bps (mid); executable ~"
                 << (tri_exec - 1.0) * 10000.0
                 << " bps after bid/ask and fees — not tradable\n";
            cout.unsetf(ios::floatfield);
        } else if (theoretical_edge && tradable) {
            cout << "Profit($1k)  : $" << fixed << setprecision(2) << (tri_exec - 1.0) * 1000.0
                 << " (executable estimate)\n";
            cout.unsetf(ios::floatfield);
        }
    }

    cout << "\n--- TRADE EXECUTION ---\n";
    vector<int> exec_tradeCost(exec_T);
    for (int t = 0; t < exec_T; ++t)
        exec_tradeCost[t] = (t % 2 == 0) ? max((int)(ask_vol * 100), 1) : max((int)(ask_vol * 50), 1);
    vector<int> exec_holdCost = build_holdCost(exec_I);

    int cost_greedy = -1, cost_dp = -1;
    if (algoC.find("Greedy") != string::npos) {
        cost_greedy = greedy_execute(exec_T, exec_I, exec_tradeCost, exec_holdCost);
        cout << "Method      : Greedy (per DecisionEngine, T*I too large for DP)\n";
        cout << "Greedy cost : " << cost_greedy << "\n";
    } else {
        cost_greedy = greedy_execute(exec_T, exec_I, exec_tradeCost, exec_holdCost);
        cost_dp = dp_execute(exec_T, exec_I, exec_tradeCost, exec_holdCost);
        cout << "Method      : DP vs Greedy baseline (per DecisionEngine)\n";
        cout << "Greedy cost : " << cost_greedy << "\n";
        cout << "DP cost     : " << cost_dp << "\n";
        cout << "DP Savings  : " << (cost_greedy - cost_dp) << " units\n";
    }
    cout << "Inventory   : " << exec_I << " units (max " << exec_T << " sells in horizon)\n";

    // Signal
    cout << "\n--- SIGNAL ---\n";
    cout << "Action : " << (bid_vol > ask_vol ? "BUY pressure - consider LONG" : "SELL pressure - consider SHORT") << "\n";

    cout << "\nNext scan in 10 seconds...\n\n";
    cout.flush();
}


int main() {
    variantforge_main();

    cout<<"\n'-----------------------------------------'\n";
    cout << "|                MENU                     |\n";
    cout << "|  1. Live Monitor Mode                   |\n";
    cout << "|  2. Stress Test Mode                    |\n";
    cout << "|  3. Both (Stress -> then Live Monitor)  |\n";
    cout<<"'-----------------------------------------'\n\n";
    cout << "Select (1/2/3): ";
    cout.flush();

    int choice = 1;
    cin >> choice;

    if (choice == 2) {
        run_stress_test();
        return 0;
    }

    if (choice == 3) {
        run_stress_test();
        cout << "\n\nStarting Live Monitor in 3 seconds...\n\n";
        cout.flush();
        _sleep(3000);
    }

    // LIVE MODE
    cout << "\n=== ARBITRAGEX LIVE MONITOR STARTED ===\n";
    cout << "Press Ctrl+C to stop.\n\n";
    cout.flush();

    int cycle = 1;
    while (true) {
        cout << "========================================\n";
        cout << "SCAN #" << cycle << "\n";
        cout << "========================================\n";
        cout.flush();

        MarketSnapshot snap;
        if (!build_market_snapshot_rest(snap)) {
            cerr << "[ERROR] Skipping scan #" << cycle << "\n";
            cycle++;
            _sleep(10000);
            continue;
        }

        run_live_scan(cycle++, snap);
        _sleep(10000);
    }

    return 0;
}
