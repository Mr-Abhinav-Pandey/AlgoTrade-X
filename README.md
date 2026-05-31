# AlgoTrade-X

Algorithmic trading **analysis prototype** in C++: fetches public Binance REST data, profiles the workload, selects a classical algorithm per sub-problem, and runs it. No ML. Windows + MinGW required for live HTTPS.

## What it does

| Mode | Purpose |
|------|---------|
| **VariantForge** (startup) | Correctness checks + micro-benchmarks comparing brute / sqrt / Fenwick, BF / SPFA, greedy / DP on **synthetic** workloads |
| **Stress test** (menu 2) | Scenarios sized to exercise each DecisionEngine branch |
| **Live monitor** (menu 1) | Polls BTCUSDT depth + cross rates every 10s; Fenwick order book, log-rate arbitrage check, execution DP |

## Architecture

```
Binance REST → parse JSON → ProblemProfile (N,Q,V,E,T,I)
                          → DecisionEngine::select_algorithm_*
                          → run selected structure (live order book = Fenwick; FX = super-source BF; execution = DP or greedy)
```

The DecisionEngine estimates operation counts against a 10⁸ ops/s budget (competitive-programming style). **Live profiles are derived from the book and graph**, not hardcoded `V*(V-1)`.

## Algorithms

| Algorithm | Problem | Where used |
|-----------|---------|------------|
| Fenwick tree | Indexed order book (1000 price buckets) | `main.cpp` `OrderBook` + live profile A when `N≥1000` |
| Brute / Sqrt / Fenwick | Range-sum stress tests | `variantforge_engine.cpp`, stress mode |
| Bellman–Ford (+ super-source for FX) | Arbitrage / negative cycle | `arbitrage_negative_cycle()` live; BF/SPFA variants in stress |
| DP / Greedy | Sell `I` units in `T` timesteps | Live + stress (`exec_I ≤ exec_T`) |

## Build (CMake)

**Prerequisites:** Windows, MinGW (C++17), CMake 3.10+, outbound HTTPS.

```bash
git clone https://github.com/Mr-Abhinav-Pandey/AlgoTrade-X.git
cd AlgoTrade-X
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

**Run** (from repo root):

```bash
echo 2 | .\bin\AlgoTradeX.exe    # stress only (offline after fetch at startup)
echo 1 | .\bin\AlgoTradeX.exe    # live monitor (Ctrl+C to stop)
```

Executable path: `bin/AlgoTradeX.exe` (not `build/bin`).

## Code::Blocks

Open `AlgoTrade-X.cbp`. Sources are under `src/`. `variantforge_engine.cpp` is included by `main.cpp` (not compiled as a separate unit).

## Limitations (intentional)

- Public REST only (no WebSocket yet; see `websocket_feed.cpp` stub)
- Live arbitrage: **theoretical** layer uses mid tickers; **executable** layer uses bid/ask (`bookTicker` + BTC depth) and 0.1% fee per leg. If bid/ask are missing, the scan still runs with theoretical output only.
- Windows-only networking (`wininet`)

## Author

Abhinav Pandey, JIIT, Noida, India
