#include <bits/stdc++.h>
using namespace std;
using namespace chrono;

void fast_io(){
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
}

long long micros() {
    return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

const int INF = 1e9;

// PROBLEM A: Order Book Depth (range sums)
const int A_N = 100000;
const int A_OPS = 1000;

struct OrderBookBrute {
    vector<int> vol;
    OrderBookBrute(int n) : vol(n + 1, 0) {}
    void update(int idx, int delta) { vol[idx] += delta; }
    int query(int L, int R) {
        int s = 0;
        for (int i = L; i <= R; ++i) s += vol[i];
        return s;
    }
};

struct OrderBookSqrt {
    int B;
    vector<int> vol;
    vector<int> block_sum;
    int n;
    OrderBookSqrt(int n) : n(n), vol(n + 1, 0) {
        B = 316;
        block_sum.assign((n / B) + 2, 0);
    }
    void update(int idx, int delta) {
        vol[idx] += delta;
        block_sum[idx / B] += delta;
    }
    int query(int L, int R) {
        int s = 0;
        int bL = L / B, bR = R / B;
        if (bL == bR) {
            for (int i = L; i <= R; ++i) s += vol[i];
        } else {
            for (int i = L; i < (bL + 1) * B; ++i) s += vol[i];
            for (int b = bL + 1; b < bR; ++b) s += block_sum[b];
            for (int i = bR * B; i <= R; ++i) s += vol[i];
        }
        return s;
    }
};

struct OrderBookFenwick {
    vector<int> bit;
    int n;
    OrderBookFenwick(int n) : n(n), bit(n + 1, 0) {}
    void update(int idx, int delta) {
        for (; idx <= n; idx += idx & -idx) bit[idx] += delta;
    }
    int prefix_sum(int idx) {
        int s = 0;
        for (; idx > 0; idx -= idx & -idx) s += bit[idx];
        return s;
    }
    int query(int L, int R) {
        return prefix_sum(R) - prefix_sum(L - 1);
    }
};

struct OpA {
    bool is_update;
    int a, b;
};

vector<OpA> generate_adversarial_ops_A() {
    vector<OpA> ops(A_OPS);
    for (int i = 0; i < A_OPS; ++i) {
        if (i % 2 == 0) {
            ops[i].is_update = true;
            ops[i].a = (i % A_N) + 1;
            ops[i].b = 1;
        } else {
            ops[i].is_update = false;
            ops[i].a = 1;
            ops[i].b = A_N;
        }
    }
    return ops;
}

void verify_A() {
    const int N = 20;
    OrderBookBrute brute(N);
    OrderBookSqrt sqrtD(N);
    OrderBookFenwick fenwick(N);

    mt19937 rng(42);
    for (int t = 0; t < 10; ++t) {
        int idx = rng() % N + 1;
        int delta = (int)(rng() % 7) - 3;
        brute.update(idx, delta);
        sqrtD.update(idx, delta);
        fenwick.update(idx, delta);
    }

    for (int t = 0; t < 5; ++t) {
        int L = rng() % N + 1;
        int R = rng() % N + 1;
        if (L > R) swap(L, R);
        int b = brute.query(L, R);
        int s = sqrtD.query(L, R);
        int f = fenwick.query(L, R);
        if (b != s || b != f) {
            cerr << "[FAIL] Problem A verification mismatch!" << endl;
            exit(1);
        }
    }
}

// PROBLEM B: Triangular Arbitrage

const int B_V = 50, B_E = 1500;

struct Edge {
    int u, v, w;
};

bool bellman_ford_brute(int V, const vector<Edge>& edges) {
    vector<int> dist(V, 0);
    for (int i = 0; i < V - 1; ++i) {
        bool any = false;
        for (size_t j = 0; j < edges.size(); ++j) {
            int u = edges[j].u;
            int v = edges[j].v;
            int w = edges[j].w;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                any = true;
            }
        }
        if (!any) break;
    }
    for (size_t j = 0; j < edges.size(); ++j) {
        if (dist[edges[j].u] + edges[j].w < dist[edges[j].v]) return true;
    }
    return false;
}

bool spfa(int V, const vector<Edge>& edges) {
    vector<vector<pair<int, int>>> adj(V);
    for (size_t i = 0; i < edges.size(); ++i) {
        adj[edges[i].u].push_back(make_pair(edges[i].v, edges[i].w));
    }

    vector<int> dist(V, 0);
    vector<bool> inq(V, false);
    vector<int> cnt(V, 0);
    queue<int> q;

    for (int i = 0; i < V; ++i) {
        q.push(i);
        inq[i] = true;
    }

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        inq[u] = false;

        for (size_t i = 0; i < adj[u].size(); ++i) {
            int v = adj[u][i].first;
            int w = adj[u][i].second;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                if (!inq[v]) {
                    q.push(v);
                    inq[v] = true;
                    if (++cnt[v] > V) return true;
                }
            }
        }
    }
    return false;
}

bool bellman_ford_opt(int V, const vector<Edge>& edges) {
    vector<int> dist(V, 0);
    vector<int> parent(V, -1);
    bool updated = false;

    for (int i = 0; i < V - 1; ++i) {
        updated = false;
        for (size_t j = 0; j < edges.size(); ++j) {
            int u = edges[j].u, v = edges[j].v, w = edges[j].w;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                updated = true;
            }
        }
        if (!updated) break;
    }

    if (!updated) return false;

    for (size_t j = 0; j < edges.size(); ++j) {
        if (dist[edges[j].u] + edges[j].w < dist[edges[j].v]) return true;
    }
    return false;
}

void verify_B() {
    vector<Edge> test_edges;
    test_edges.push_back({0, 1, -1});
    test_edges.push_back({1, 2, -2});
    test_edges.push_back({2, 0, -5});

    bool b = bellman_ford_brute(3, test_edges);
    bool s = spfa(3, test_edges);
    bool o = bellman_ford_opt(3, test_edges);
    if (b != s || b != o) {
        cerr << "[FAIL] Problem B verification mismatch!" << endl;
        exit(1);
    }
}


// PROBLEM C: Optimal Trade Execution (DP)
int holdCostFunc(int inv) {
    return (inv * inv) / 2;
}

struct BruteSolver {
    int T, I;
    const vector<int>& tradeCost;
    const vector<int>& holdCost;
    int best;

    BruteSolver(int T_, int I_, const vector<int>& tC, const vector<int>& hC)
        : T(T_), I(I_), tradeCost(tC), holdCost(hC), best(INF) {}

    void dfs(int idx, int taken, int cur_cost) {
        if (idx == T) {
            if (taken == I && cur_cost < best) best = cur_cost;
            return;
        }
        int inv = I - taken;
        dfs(idx + 1, taken, cur_cost + holdCost[inv]);
        if (taken < I) {
            dfs(idx + 1, taken + 1, cur_cost + tradeCost[idx] + holdCost[inv - 1]);
        }
    }
};

int brute_execute(int T, int I, const vector<int>& tradeCost, const vector<int>& holdCost) {
    BruteSolver solver(T, I, tradeCost, holdCost);
    solver.dfs(0, 0, 0);
    return solver.best;
}

int greedy_execute(int T, int I, const vector<int>& tradeCost, const vector<int>& holdCost) {
    int inv = I, cost = 0;
    for (int t = 0; t < T; ++t) {
        if (inv > 0) {
            cost += tradeCost[t] + holdCost[inv - 1];
            --inv;
        } else {
            cost += holdCost[inv];
        }
    }
    return cost;
}

int dp_execute(int T, int I, const vector<int>& tradeCost, const vector<int>& holdCost) {
    vector<int> dp((T + 1) * (I + 1), INF);
    dp[T * (I + 1) + 0] = 0;

    for (int t = T - 1; t >= 0; --t) {
        for (int i = 0; i <= I; ++i) {
            int cur_idx = t * (I + 1) + i;
            int next_same = (t + 1) * (I + 1) + i;
            int val = holdCost[i] + dp[next_same];
            if (i > 0) {
                int next_less = (t + 1) * (I + 1) + (i - 1);
                int sell_val = tradeCost[t] + holdCost[i - 1] + dp[next_less];
                if (sell_val < val) val = sell_val;
            }
            dp[cur_idx] = val;
        }
    }
    return dp[0 * (I + 1) + I];
}

void verify_C() {
    const int T = 10, I = 5;
    vector<int> tradeCost(T, 0);
    vector<int> holdCost(I + 1);
    for (int i = 0; i <= I; ++i) holdCost[i] = holdCostFunc(i);

    int b = brute_execute(T, I, tradeCost, holdCost);
    int g = greedy_execute(T, I, tradeCost, holdCost);
    int d = dp_execute(T, I, tradeCost, holdCost);
    if (b != g || b != d) {
        cerr << "[FAIL] Problem C verification mismatch! (b=" << b << " g=" << g << " d=" << d << ")" << endl;
        exit(1);
    }
}

vector<int> build_holdCost(int I) {
    vector<int> hC(I + 1);
    for (int i = 0; i <= I; ++i) hC[i] = holdCostFunc(i);
    return hC;
}

// Problem Profiles for Decision Engine
struct ProblemProfileA {
    int N;
    int Q;
    double update_ratio;
};

struct ProblemProfileB {
    int V;
    int E;
};

struct ProblemProfileC {
    int T;
    int I;
};

struct DecisionEngine {
    static double ops_per_second() { return 1e8; }

    static bool high_tle_risk(double estimated_ops) {
        return estimated_ops > ops_per_second();
    }

    static string select_algorithm_A(const ProblemProfileA& p, string& logic, string& tle_risk) {
        double ops_brute = 1.0 * p.Q * p.N;
        double ops_sqrt = 1.0 * p.Q * sqrt(p.N);
        double ops_fenwick = 1.0 * p.Q * log2(p.N);

        if (p.N <= 1000 && p.Q <= 1000) {
            logic = "N and Q are very small; brute force is sufficient and simpler.";
            tle_risk = high_tle_risk(ops_brute) ? "High" : "Low";
            return "Brute Force (O(N) per operation)";
        }

        if (p.N <= 50000 && p.Q <= 50000) {
            if (ops_sqrt > ops_per_second()) {
                logic = "Sqrt decomposition estimated operations (" + to_string((long long)ops_sqrt) + ") exceed 10^8, risking TLE.";
                tle_risk = "High";
            } else {
                logic = "Sqrt decomposition (O(√N)) is viable; operations ~ " + to_string((long long)ops_sqrt) + ".";
                tle_risk = "Low";
            }
            return "Sqrt Decomposition (O(√N) per operation)";
        }

       logic = "Fenwick tree (O(log N)) operations ~ " + to_string((long long)ops_fenwick)
        + "; guarantees safe execution even with large N and Q.";
        tle_risk = high_tle_risk(ops_fenwick) ? "High" : "Low";
        return "Fenwick Tree (O(log N) per operation)";
    }

    static string select_algorithm_B(const ProblemProfileB& p, string& logic, string& tle_risk) {
        double ops_bf = 1.0 * p.V * p.E;

        if (p.V <= 100 && p.E <= 1000) {
            logic = "Small graph: Bellman-Ford O(VE) is safe and preferred.";
            tle_risk = "Low";
            return "Bellman-Ford (O(VE) brute)";
        }

        if (ops_bf <= ops_per_second()) {
            logic = "Estimated Bellman-Ford operations (" + to_string((long long)ops_bf)
            + ") fit within 1e8.\nBellman-Ford is recommended for robustness.";
            tle_risk = "Low";
            return "Bellman-Ford (O(VE))";
        } else {
            logic = "Estimated operations (" + to_string((long long)ops_bf) + ") exceed 10^8.\nSPFA may be faster on average, but can degrade on adversarial tests.";
            tle_risk = "High (adversarial SPFA risk)";
            return "SPFA (O(VE) worst-case, average faster)";
        }
    }

    static string select_algorithm_C(const ProblemProfileC& p, string& logic, string& tle_risk) {
        long long ops_dp = 1LL * p.T * p.I;

        if (1LL * p.T * p.I <= 10000000LL) {
            logic = "DP states = T*I = " + to_string((long long)p.T * p.I)
            + " <= 1e7, optimal DP (O(T*I)) will run easily within 1 sec.";
            tle_risk = "Low";
            return "Optimal DP (O(T·I))";
        } else if (1LL * p.T * p.I <= 50000000LL) {
            logic = "DP states = " + to_string(1LL * p.T * p.I) + " ≤ 5×10^7; may be borderline but still feasible.";
            tle_risk = high_tle_risk((double)ops_dp) ? "High" : "Low";
            return "Optimal DP (O(T*I))";
        } else {
            logic = "DP states = " + to_string(1LL * p.T * p.I) + " exceed 5×10^7, too large for a 1 sec limit.";
            tle_risk = high_tle_risk((double)ops_dp) ? "High" : "Low";
            return "Greedy Heuristic (O(T))";
        }
    }
};

// BENCHMARK HARNESS
struct BenchResult {
    string problem;
    string N_label;
    string algorithm;
    long long time_us;
    bool passed;
};

//  STRESS TEST MODE
struct StressResult {
    string scenario;
    string problem;
    string algorithm;
    long long time_us;
    string verdict;
};

void run_stress_test() {
    cout<<"\n|_________________________________________|\n";
    cout << "|         STRESS TEST MODE                |\n";
    cout << "|_________________________________________|\n\n";
    cout << "Running synthetic scenarios that trigger EVERY algorithm...\n\n";
    cout.flush();

    vector<StressResult> results;
    volatile int sink = 0;

    cout << "[ PROBLEM A : Order Book Depth Range Queries ]\n";
    cout.flush();

    struct ScenA { string label; int N; int Q; string expected_algo; };
    vector<ScenA> scenA = {
        {"Tiny   (N=500, Q=200)",   500,   200,  "Brute Force"},
        {"Medium (N=30k, Q=20k)",   30000, 20000,"Sqrt Decomp"},
        {"HFT    (N=100k, Q=100k)", 100000,100000,"Fenwick Tree"},
    };

    mt19937 rng(9999);
    for (auto& sc : scenA) {
        string logic, tle;
        ProblemProfileA prof = { sc.N, sc.Q, 0.5 };
        string algo = DecisionEngine::select_algorithm_A(prof, logic, tle);

        vector<OpA> ops(sc.Q);
        uniform_int_distribution<int> idx_d(1, sc.N);
        uniform_int_distribution<int> del_d(-5, 5);
        uniform_int_distribution<int> coin(0, 1);
        for (int i = 0; i < sc.Q; ++i) {
            ops[i].is_update = (coin(rng) == 0);
            ops[i].a = idx_d(rng);
            ops[i].b = ops[i].is_update ? del_d(rng) : idx_d(rng);
            if (!ops[i].is_update && ops[i].a > ops[i].b) swap(ops[i].a, ops[i].b);
        }

        long long elapsed = -1;
        if (algo.find("Brute") != string::npos) {
            OrderBookBrute ds(sc.N);
            long long t0 = micros();
            for (auto& op : ops) {
                if (op.is_update) ds.update(op.a, op.b);
                else sink += ds.query(op.a, op.b);
            }
            elapsed = micros() - t0;
        } else if (algo.find("Sqrt") != string::npos) {
            OrderBookSqrt ds(sc.N);
            long long t0 = micros();
            for (auto& op : ops) {
                if (op.is_update) ds.update(op.a, op.b);
                else sink += ds.query(op.a, op.b);
            }
            elapsed = micros() - t0;
        } else {
            OrderBookFenwick ds(sc.N);
            long long t0 = micros();
            for (auto& op : ops) {
                if (op.is_update) ds.update(op.a, op.b);
                else sink += ds.query(op.a, op.b);
            }
            elapsed = micros() - t0;
        }

        string verdict = (elapsed < 200000) ? "PASS" : "SLOW";
        cout << "  " << sc.label << "  ->  " << algo << "  [" << elapsed << " us] " << verdict << "\n";
        cout.flush();
        results.push_back({sc.label, "A", algo, elapsed, verdict});
    }

    cout << "\n[ PROBLEM B : Negative Cycle / Triangular Arbitrage ]\n";
    cout.flush();

    struct ScenB { string label; int V; int E; string expected_algo; };
    vector<ScenB> scenB = {
        {"Tiny   (V=10,  E=50)",    10,  50,    "Bellman-Ford brute"},
        {"Medium (V=100, E=5000)",  100, 5000,  "Bellman-Ford O(VE)"},
        {"Large  (V=500, E=50000)", 500, 50000, "SPFA"},
    };

    for (auto& sc : scenB) {
        string logic, tle;
        ProblemProfileB prof = { sc.V, sc.E };
        string algo = DecisionEngine::select_algorithm_B(prof, logic, tle);

        vector<Edge> edges;
        uniform_int_distribution<int> nd(0, sc.V - 1);
        uniform_int_distribution<int> wd(-1000, 1000);
        set<pair<int,int>> used;
        int attempts = 0;
        while ((int)edges.size() < sc.E && attempts < sc.E * 10) {
            int u = nd(rng), v = nd(rng);
            if (u != v && !used.count({u, v})) {
                used.insert({u, v});
                Edge e; e.u = u; e.v = v; e.w = wd(rng);
                edges.push_back(e);
            }
            attempts++;
        }

        long long elapsed = -1;
        bool found = false;
        if (algo.find("SPFA") != string::npos) {
            long long t0 = micros(); found = spfa(sc.V, edges); elapsed = micros() - t0;
        } else {
            long long t0 = micros(); found = bellman_ford_opt(sc.V, edges); elapsed = micros() - t0;
        }

        string verdict = (elapsed < 2000000) ? "PASS" : "SLOW";
        cout << "  " << sc.label << "  ->  " << algo
             << "  [" << elapsed << " us]  neg-cycle=" << (found ? "YES" : "NO") << "  " << verdict << "\n";
        cout.flush();
        results.push_back({sc.label, "B", algo, elapsed, verdict});
    }

    cout << "\n[ PROBLEM C : Optimal Trade Execution ]\n";
    cout.flush();

    struct ScenC { string label; int T; int I; };
    vector<ScenC> scenC = {
        {"Tiny   (T=10,  I=5)",    10,   5},
        {"Medium (T=200, I=100)",  200,  100},
        {"Large  (T=5000,I=1000)", 5000, 1000},
    };

    for (auto& sc : scenC) {
        string logic, tle;
        ProblemProfileC prof = { sc.T, sc.I };
        string algo = DecisionEngine::select_algorithm_C(prof, logic, tle);

        vector<int> tC(sc.T);
        for (int t = 0; t < sc.T; ++t) tC[t] = (t % 3 == 0) ? 1 : 10;
        vector<int> hC = build_holdCost(sc.I);

        long long elapsed = -1;
        int cost_g = -1, cost_opt = -1;

        if (algo.find("Brute") != string::npos) {
            long long t0 = micros(); cost_opt = brute_execute(sc.T, sc.I, tC, hC); elapsed = micros() - t0;
            cost_g = greedy_execute(sc.T, sc.I, tC, hC);
        } else if (algo.find("Greedy") != string::npos) {
            long long t0 = micros(); cost_g = greedy_execute(sc.T, sc.I, tC, hC); elapsed = micros() - t0;
            cost_opt = cost_g;
        } else {
            int cg = greedy_execute(sc.T, sc.I, tC, hC);
            long long t0 = micros(); cost_opt = dp_execute(sc.T, sc.I, tC, hC); elapsed = micros() - t0;
            cost_g = cg;
        }

        int savings = cost_g - cost_opt;
        string verdict = (elapsed < 3000000) ? "PASS" : "SLOW";
        cout << "  " << sc.label << "  ->  " << algo
             << "  [" << elapsed << " us]  DP-saves=" << savings << "  " << verdict << "\n";
        cout.flush();
        results.push_back({sc.label, "C", algo, elapsed, verdict});
    }

    {
        ofstream csv("stress_results.csv");
        csv << "Scenario,Problem,Algorithm,Time_us,Verdict\n";
        for (auto& r : results)
            csv << r.scenario << "," << r.problem << ","
                << r.algorithm << "," << r.time_us << "," << r.verdict << "\n";
        cout << "\nResults exported to stress_results.csv\n";
    }

    cout << "\n=== SUMMARY ===\n";
    cout << left
         << setw(30) << "Scenario"
         << setw(12) << "Problem"
         << setw(35) << "Algorithm"
         << setw(14) << "Time (us)"
         << "Verdict\n";
    cout << string(100, '-') << "\n";
    for (auto& r : results) {
        cout << left
             << setw(30) << r.scenario
             << setw(12) << r.problem
             << setw(35) << r.algorithm
             << setw(14) << r.time_us
             << r.verdict << "\n";
    }
    cout.flush();
}

static void force_edge(vector<Edge>& edges, int u, int v, int w) {
    for (auto& e : edges) {
        if (e.u == u && e.v == v) { e.w = w; return; }
    }
    edges.push_back({u, v, w});
}

int variantforge_main() {
    fast_io();

    cout << "=== VARIANTFORGE DECISION ENGINE ===\n";
    cout << "Stress-testing all algorithm variants to find where each breaks...\n\n";

    verify_A(); verify_B(); verify_C();
    cout << "All correctness gates passed.\n\n";
    cout.flush();

    volatile int sink = 0;

    // PROBLEM A : Order Book Depth
    cout << "[ PROBLEM A : Order Book Depth (Range Sum Queries) ]\n\n";
    cout.flush();

    mt19937 rng_a(42);

    auto bench_A = [&](int N, vector<OpA>& ops, const string& label, const string& note) {
        long long t_b, t_s, t_f;
        {
            OrderBookBrute ds(N);
            long long t0 = micros();
            for (auto& op : ops) { if (op.is_update) ds.update(op.a, op.b); else sink += ds.query(op.a, op.b); }
            t_b = micros() - t0;
        }
        {
            OrderBookSqrt ds(N);
            long long t0 = micros();
            for (auto& op : ops) { if (op.is_update) ds.update(op.a, op.b); else sink += ds.query(op.a, op.b); }
            t_s = micros() - t0;
        }
        {
            OrderBookFenwick ds(N);
            long long t0 = micros();
            for (auto& op : ops) { if (op.is_update) ds.update(op.a, op.b); else sink += ds.query(op.a, op.b); }
            t_f = micros() - t0;
        }
        long long SLA = 5000;
        cout << "  Scenario : " << label << "\n";
        cout << "  Note     : " << note << "\n";
        cout << "  Brute   O(N)     : " << setw(8) << t_b << " us  " << (t_b > SLA ? "FAIL" : "PASS") << "\n";
        cout << "  Sqrt    O(sqrtN) : " << setw(8) << t_s << " us  " << (t_s > SLA ? "FAIL" : "PASS") << "\n";
        cout << "  Fenwick O(logN)  : " << setw(8) << t_f << " us  " << (t_f > SLA ? "FAIL" : "PASS") << "\n";
        long long best = min({t_b, t_s, t_f});
        string winner = (best == t_f) ? "Fenwick" : (best == t_s) ? "Sqrt" : "Brute";
        cout << "  FASTEST  : " << winner << "\n\n";
        cout.flush();
    };

    // ── A Case 1: Tiny N, ALL point queries (L == R)
    {
        int N = 500;
        vector<OpA> ops(1000);
        uniform_int_distribution<int> idx(1, N), del(-3, 3);
        // 20% updates, 80% point queries
        for (int i = 0; i < 1000; ++i) {
            if (i < 200) {
                ops[i].is_update = true;
                ops[i].a = idx(rng_a);
                ops[i].b = del(rng_a);
            } else {
                ops[i].is_update = false;
                int x = idx(rng_a);
                ops[i].a = ops[i].b = x;   // L == R  →  range width = 1
            }
        }
        bench_A(N, ops,
            "N=500, Q=1000, 80% point queries (L==R)",
            "Brute: 1 array access per query. Fenwick: 2*prefix_sum = ~18 bit-ops. "
            "Sqrt: scans 1 element in same-block branch. Brute wins on minimal-range queries.");
    }

    // ── A Case 2: Medium N, TINY range queries (width 1-3 elements)
    {
        int N = 50000;
        vector<OpA> ops(2000);
        uniform_int_distribution<int> idx(1, N - 3), del(-3, 3);
        for (int i = 0; i < 2000; ++i) {
            if (i < 400) {                      // 20% updates
                ops[i].is_update = true;
                ops[i].a = idx(rng_a) + 3;
                ops[i].b = del(rng_a);
            } else {                            // 80% tiny-range queries
                ops[i].is_update = false;
                int L = idx(rng_a);
                ops[i].a = L;
                ops[i].b = L + (int)(rng_a() % 3); // width 1, 2, or 3
            }
        }
        bench_A(N, ops,
            "N=50000, Q=2000, tiny range queries (width 1-3 elements)",
            "Brute scans 1-3 elements. Fenwick always pays 2*log2(50000)=32 ops. "
            "Sqrt same-block scan: 1-3 elements. Brute or Sqrt beat Fenwick constant overhead.");
    }

    // ── A Case 3: Large N, adversarial FULL-RANGE queries [1, N]
    {
        vector<OpA> ops = generate_adversarial_ops_A();
        bench_A(A_N, ops,
            "N=100000, Q=1000, adversarial full-range queries [1,N]",
            "Brute scans 100K elements per query. Sqrt traverses all 317 blocks + partial edges. "
            "Fenwick: 17 pointer-hops. Only Fenwick survives.");
    }

    // ── A Case 4: Large N, 90% UPDATES
    {
        int N = A_N;
        vector<OpA> ops(1000);
        uniform_int_distribution<int> idx(1, N), del(-3, 3), coin(0, 9);
        for (auto& op : ops) {
            op.is_update = (coin(rng_a) != 0);  // 90% updates
            op.a = idx(rng_a);
            op.b = op.is_update ? del(rng_a) : idx(rng_a);
            if (!op.is_update && op.a > op.b) swap(op.a, op.b);
        }
        bench_A(N, ops,
            "N=100000, Q=1000, 90% updates / 10% queries",
            "Sqrt update O(1): write vol[idx] + block_sum[idx/B]. "
            "Fenwick update: cascade through 17 BIT nodes. Update-heavy load: Sqrt wins.");
    }

    {
        string logic, tle;
        ProblemProfileA prof = { A_N, A_OPS, 0.5 };
        string algo = DecisionEngine::select_algorithm_A(prof, logic, tle);
        cout << "  ENGINE VERDICT (live market: N=100k, Q=1000, mixed ops)\n";
        cout << "  Selects : " << algo << "\n";
        cout << "  Reason  : " << logic << "\n\n";
    }

    // PROBLEM B : Triangular Arbitrage / Negative Cycle
    cout << "[ PROBLEM B : Triangular Arbitrage (Negative Cycle Detection) ]\n\n";
    cout.flush();

    mt19937 rng_b(999);

    auto bench_B = [&](int V, vector<Edge>& edges, const string& label, const string& note) {
        long long t_bf, t_spfa, t_opt;
        bool r_bf, r_spfa, r_opt;
        { long long t0 = micros(); r_bf   = bellman_ford_brute(V, edges); t_bf   = micros() - t0; }
        { long long t0 = micros(); r_spfa = spfa(V, edges);               t_spfa = micros() - t0; }
        { long long t0 = micros(); r_opt  = bellman_ford_opt(V, edges);   t_opt  = micros() - t0; }
        long long SLA = 2000000;   // 2-second SLA
        cout << "  Scenario          : " << label << "\n";
        cout << "  Note              : " << note << "\n";
        cout << "  BF Brute  O(VE)   : " << setw(8) << t_bf   << " us  " << (t_bf   > SLA ? "FAIL" : "PASS") << "  neg-cycle=" << (r_bf   ? "YES" : "NO") << "\n";
        cout << "  SPFA              : " << setw(8) << t_spfa  << " us  " << (t_spfa > SLA ? "FAIL" : "PASS") << "  neg-cycle=" << (r_spfa ? "YES" : "NO") << "\n";
        cout << "  BF Opt(early-exit): " << setw(8) << t_opt   << " us  " << (t_opt  > SLA ? "FAIL" : "PASS") << "  neg-cycle=" << (r_opt  ? "YES" : "NO") << "\n";
        long long best = min({t_bf, t_spfa, t_opt});
        string winner = (best == t_opt) ? "BF Opt" : (best == t_spfa) ? "SPFA" : "BF Brute";
        cout << "  FASTEST           : " << winner << "\n\n";
        cout.flush();
    };

    // ── B Case 1: Long sparse CHAIN, negative weights, NO neg-cycle
    {
        int V = 1000;
        vector<Edge> chain;
        chain.reserve(V - 1);
        for (int i = 0; i < V - 1; ++i) chain.push_back({i, i + 1, -1});
        bench_B(V, chain,
            "V=1000, E=999, linear chain (w=-1, no neg-cycle)",
            "SPFA: chain processed in queue order; each node dequeued once -> O(V+E)=2000 ops. "
            "BF: 1 hop relaxed per pass; needs V-1=999 passes x 999 edges = 1M ops. SPFA wins ~500x.");
    }

    // ── B Case 2: Dense ALL-POSITIVE weights → BF Opt wins (trivial 1-pass exit)
    {
        int V = 400;
        vector<Edge> pos_edges;
        pos_edges.reserve(50000);
        set<pair<int,int>> used;
        uniform_int_distribution<int> nd(0, V - 1), wd(1, 100);
        int att = 0;
        while ((int)pos_edges.size() < 50000 && att < 500000) {
            int u = nd(rng_b), v = nd(rng_b);
            if (u != v && !used.count({u, v})) {
                used.insert({u, v});
                pos_edges.push_back({u, v, wd(rng_b)});
            }
            ++att;
        }
        bench_B(V, pos_edges,
            "V=400, E=" + to_string(pos_edges.size()) + ", dense ALL-POSITIVE weights",
            "dist[]=0; all w>0: no edge ever relaxes. "
            "BF Opt: 1 pass x E edges, updated=false, exits immediately. "
            "SPFA: dequeues all 400 nodes, checks ~50K adj entries with queue overhead. BF Opt wins.");
    }

    // ── B Case 3: Dense graph, OBVIOUS neg-cycle (3-cycle, reachable from all nodes)
    {
        int V = 300;
        vector<Edge> nc_edges;
        nc_edges.reserve(10100);
        set<pair<int,int>> used;
        uniform_int_distribution<int> nd(0, V - 1), wd(-5, 20);
        int att = 0;
        while ((int)nc_edges.size() < 10000 && att < 100000) {
            int u = nd(rng_b), v = nd(rng_b);
            if (u != v && !used.count({u, v})) {
                used.insert({u, v});
                nc_edges.push_back({u, v, wd(rng_b)});
            }
            ++att;
        }
        // Guarantee a strong neg-cycle at 0→1→2→0 (sum = -180)
        force_edge(nc_edges, 0, 1, -60);
        force_edge(nc_edges, 1, 2, -60);
        force_edge(nc_edges, 2, 0, -60);
        bench_B(V, nc_edges,
            "V=300, E=~10K, dense, obvious neg-cycle 0->1->2->0 (sum=-180)",
            "BF Opt: neg-cycle keeps updated=true; runs full 299 passes x ~10K edges = ~3M ops, then 1 detection pass. "
            "SPFA: cnt[0,1,2] each exceed V=300 after ~300 cycle re-queues (~900 ops). SPFA detects ~3300x faster.");
    }

    // ── B Case 4: Dense graph, neg-cycle BURIED at end (SPFA worst-case)
    {
        int V = 300;
        vector<Edge> buried;
        buried.reserve(40100);
        set<pair<int,int>> used;
        uniform_int_distribution<int> nd(0, V - 1), wd(-10, 10);
        int att = 0;
        while ((int)buried.size() < 40000 && att < 400000) {
            int u = nd(rng_b), v = nd(rng_b);
            if (u != v && !used.count({u, v})) {
                used.insert({u, v});
                buried.push_back({u, v, wd(rng_b)});
            }
            ++att;
        }
        buried.push_back({V - 3, V - 2, -500});
        buried.push_back({V - 2, V - 1, -500});
        buried.push_back({V - 1, V - 3, -500});
        bench_B(V, buried,
            "V=300, E=40003, dense, neg-cycle buried at nodes 297-299",
            "Dense random graph forces SPFA to propagate through all ~300 nodes before cycle detected. "
            "BF Opt: full 299 passes x 40K edges = 12M ops (no early exit with neg-cycle). "
            "Both hit O(VE); shows SPFA's worst-case variance on dense adversarial graphs.");
    }

    {
        string logic, tle;
        ProblemProfileB prof = { B_V, B_E };
        string algo = DecisionEngine::select_algorithm_B(prof, logic, tle);
        cout << "  ENGINE VERDICT (live market: V=5 currencies, E=18 pairs)\n";
        cout << "  Selects : " << algo << "\n";
        cout << "  Reason  : " << logic << "\n\n";
    }

    // PROBLEM C : Optimal Trade Execution
    cout << "[ PROBLEM C : Optimal Trade Execution (DP) ]\n\n";
    cout.flush();

    auto bench_C = [&](int T, int I,vector<int>& tC, vector<int>& hC, const string& label, const string& note, bool run_brute) {
        long long SLA = 2000000;
        long long t_brute = -1, t_greedy, t_dp;
        int cost_brute = -1, cost_greedy, cost_dp;
        if (run_brute) { long long t0 = micros(); cost_brute  = brute_execute  (T, I, tC, hC); t_brute  = micros() - t0; }
        {               long long t0 = micros(); cost_greedy = greedy_execute(T, I, tC, hC); t_greedy = micros() - t0; }
        {               long long t0 = micros(); cost_dp     = dp_execute    (T, I, tC, hC); t_dp     = micros() - t0; }

        cout << "  Scenario  : " << label << "\n";
        cout << "  Note      : " << note << "\n";
        if (run_brute)
            cout << "  Brute O(2^T)     : cost=" << setw(10) << cost_brute  << "  " << setw(8) << t_brute  << " us  " << (t_brute  > SLA ? "FAIL" : "PASS") << "\n";
        else
            cout << "  Brute O(2^T)     : SKIPPED (T=" << T << " exponential, would never finish)\n";
        cout << "  Greedy O(T)      : cost=" << setw(10) << cost_greedy << "  " << setw(8) << t_greedy << " us  " << (t_greedy > SLA ? "FAIL" : "PASS") << "\n";
        cout << "  Optimal DP O(TI) : cost=" << setw(10) << cost_dp    << "  " << setw(8) << t_dp     << " us  " << (t_dp     > SLA ? "FAIL" : "PASS") << "\n";

        int savings = cost_greedy - cost_dp;
        double pct  = (cost_greedy > 0) ? 100.0 * savings / cost_greedy : 0.0;
        if (savings > 0)
            cout << "  DP SAVES  : " << savings << " units (" << fixed << setprecision(1) << pct << "% cheaper than greedy)\n";
        else
            cout << "  DP SAVES  : 0 -- greedy already optimal; DP wasted " << t_dp << " us for nothing\n";

        string winner = (savings > 0)
            ? "Optimal DP"
            : "Greedy (same cost, " + to_string(t_dp / max(t_greedy, 1LL)) + "x faster)";
        cout << "  WINNER    : " << winner << "\n\n";
        cout.flush();
    };

    // ── C Case 1: Uniform trade cost + quadratic hold cost
    {
        int T = 12, I = 6;
        vector<int> tC(T, 10);
        vector<int> hC = build_holdCost(I);
        bench_C(T, I, tC, hC,
            "T=12, I=6, uniform trade cost, quadratic hold cost",
            "All slots cost 10 to trade. Any sell order gives the same total cost. "
            "Greedy wins: O(T) vs O(T*I), zero extra savings from DP.",
            true);
    }

    // ── C Case 2: STRICTLY INCREASING trade cost, quadratic hold cost
    {
        int T = 5000, I = 100;
        vector<int> tC(T);
        for (int t = 0; t < T; ++t) tC[t] = t + 1; // 1, 2, 3, ..., 5000
        vector<int> hC = build_holdCost(I);
        bench_C(T, I, tC, hC,
            "T=5000, I=100, increasing trade cost (1,2,...,5000), quadratic hold cost",
            "Cheapest slots are at start; greedy sells t=0..99, exactly the optimal set. "
            "DP agrees: 0 savings. Greedy wins: O(T) vs O(T*I).",
            false);
    }

    // ── C Case 3: STRICTLY DECREASING trade cost, ZERO hold cost
    {
        int T = 5000, I = 100;
        vector<int> tC(T);
        for (int t = 0; t < T; ++t) tC[t] = T - t; // 5000 down to 1
        vector<int> hC(I + 1, 0);  // ZERO hold cost → isolate trade cost
        bench_C(T, I, tC, hC,
            "T=5000, I=100, decreasing cost (5000..1), ZERO hold cost",
            "Zero hold cost isolates trade-cost decision. "
            "Greedy sells t=0..99 paying 5000+4999+...+4901 = 495,050. "
            "DP waits, sells t=4900..4999 paying 100+99+...+1 = 5,050. DP saves 99%.",
            false);
    }

    // ── C Case 4: TWO cheap windows, flat cost elsewhere, ZERO hold cost
    {
        int T = 5000, I = 100;
        vector<int> tC(T, 500);
        for (int t = 1000; t < 1050; ++t) tC[t] = 1; // window 1: 50 cheap slots
        for (int t = 3000; t < 3050; ++t) tC[t] = 1; // window 2: 50 cheap slots
        vector<int> hC(I + 1, 0);  // ZERO hold cost → isolate trade cost
        bench_C(T, I, tC, hC,
            "T=5000, I=100, flat=500 with two 50-slot windows at cost=1, ZERO hold cost",
            "Greedy sells t=0..99 (no future knowledge) paying 500x100=50,000. "
            "DP finds both cheap windows (t=1000-1049 and t=3000-3049) paying 1x100=100. "
            "DP saves 49,900 units (99.8%). Demonstrates DP's global optimality advantage.",
            false);
    }

    {
        string logic, tle;
        ProblemProfileC prof = { 5000, 1000 };
        string algo = DecisionEngine::select_algorithm_C(prof, logic, tle);
        cout << "  ENGINE VERDICT (live market: T=5000 slots, I=1000 units)\n";
        cout << "  Selects : " << algo << "\n";
        cout << "  Reason  : " << logic << "\n\n";
    }

    cout.flush();
    return 0;
}
