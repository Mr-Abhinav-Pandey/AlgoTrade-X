#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <exception>
#include <iomanip>
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

OrderBook load_order_book(const string& json, double current_price) {
    OrderBook book(current_price - 500.0, 1.0, 1000);
    book.mid_price = current_price;
    for (auto& b : parse_orders(json, "bids")) book.add_bid(b.first, b.second);
    for (auto& a : parse_orders(json, "asks")) book.add_ask(a.first, a.second);
    return book;
}


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

void run_live_scan(int cycle) {
    cout << "========================================\n";
    cout << "SCAN #" << cycle << "\n";
    cout << "========================================\n";
    cout.flush();   // <── FIX: flush before slow network calls

    string price_json = fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT");
    if (price_json.empty()) {
        cerr << "[ERROR] Price fetch failed; skipping scan #" << cycle << "\n";
        return;
    }
    double current_price = 0.0;
    try {
        current_price = parse_double(price_json, "price");
    } catch (const exception& ex) {
        cerr << "[ERROR] Price parse failed: " << ex.what() << "\n";
        return;
    }

    string book_json = fetch_binance("https://api.binance.com/api/v3/depth?symbol=BTCUSDT&limit=20");
    if (book_json.empty()) {
        cerr << "[ERROR] Depth fetch failed; skipping scan #" << cycle << "\n";
        return;
    }
    OrderBook book = load_order_book(book_json, current_price);

    double lo = current_price - 100, hi = current_price + 100;
    double bid_vol = book.bid_volume(lo, current_price);
    double ask_vol = book.ask_volume(current_price, hi);

    const int grid_buckets = 1000;
    auto bid_levels = parse_orders(book_json, "bids");
    auto ask_levels = parse_orders(book_json, "asks");
    const int live_N = grid_buckets;
    const int live_Q = max((int)(bid_levels.size() + ask_levels.size()), 1);

    const int exec_T = 20;
    const int exec_I = min(max((int)(bid_vol * 10.0 + 0.5), 1), exec_T);

    // Arbitrage scan (build graph before profiling problem B)
    cout << "\n--- ARBITRAGE SCAN ---\n";
    double btcusdt = 0, ethusdt = 0, bnbusdt = 0, ethbtc = 0, bnbbtc = 0;
    try {
        btcusdt = parse_double(fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT"), "price");
        ethusdt = parse_double(fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=ETHUSDT"), "price");
        bnbusdt = parse_double(fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BNBUSDT"), "price");
        ethbtc  = parse_double(fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=ETHBTC"),  "price");
        bnbbtc  = parse_double(fetch_binance("https://api.binance.com/api/v3/ticker/price?symbol=BNBBTC"),  "price");
    } catch (const exception& ex) {
        cerr << "[ERROR] Cross-rate fetch/parse failed: " << ex.what() << "\n";
        return;
    }

    // 0=USDT, 1=BTC, 2=ETH, 3=BNB — matches auditable triangular cycles below.
    const int live_V = 4;
    vector<Edge> arb_edges;
    arb_edges.push_back(make_rate_edge(0, 1, 1.0 / btcusdt));
    arb_edges.push_back(make_rate_edge(1, 0, btcusdt));
    arb_edges.push_back(make_rate_edge(0, 2, 1.0 / ethusdt));
    arb_edges.push_back(make_rate_edge(2, 0, ethusdt));
    arb_edges.push_back(make_rate_edge(0, 3, 1.0 / bnbusdt));
    arb_edges.push_back(make_rate_edge(3, 0, bnbusdt));
    arb_edges.push_back(make_rate_edge(1, 2, 1.0 / ethbtc));
    arb_edges.push_back(make_rate_edge(2, 1, ethbtc));
    arb_edges.push_back(make_rate_edge(1, 3, 1.0 / bnbbtc));
    arb_edges.push_back(make_rate_edge(3, 1, bnbbtc));

    vector<Edge> arb_graph = dedupe_fx_edges(arb_edges);
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

    bool graph_cycle = arbitrage_negative_cycle(live_V, arb_graph);
    double tri_gain = best_triangular_gain(btcusdt, ethusdt, bnbusdt, ethbtc, bnbbtc);
    const double fee_buffer = 1.0005;
    bool profitable = tri_gain > fee_buffer;

    cout << "Detector   : log-rate Bellman-Ford (super-source), edges=" << live_E << "\n";
    cout << "Neg-cycle  : " << (graph_cycle ? "yes" : "no") << "  (graph model; stress uses " << algoB << ")\n";
    cout << "Best tri   : " << fixed << setprecision(6) << tri_gain
         << "x (USDT->BTC->alt->USDT, auditable)\n";
    cout.unsetf(ios::floatfield);
    cout << "Opportunity: " << (profitable ? "YES (triangular gain > fees)" : "NO - Market efficient") << "\n";
    if (profitable) {
        cout << "Profit($1k): $" << (tri_gain - 1.0) * 1000.0 << "\n";
    } else if (graph_cycle && !profitable) {
        cout << "Note       : graph negative cycle without auditable triangle > fees (check rates)\n";
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
        run_live_scan(cycle++);
        _sleep(10000);
    }

    return 0;
}
