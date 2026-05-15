# AlgoTrade-X
It is a algorithm based recommendation engine prototype in domain of trading. It has a stress mode to test the algorithms breaking point and a real data analysis from binance data every 10 secs.

## Overview
It is trying to create a high frequency trading advisor without using AI or ML models. We are trying to implement a competitive programming approach on how to fetch, parse and analyze data within the real time constraints using cpp language.

## Features
- 2 modes (Stress mode & Live Monitor)
- Tries multiple algorithms(brute force, optimized, named or specific)
- Faster than normal aiml projects in python

## Tech Stack (currently)
- Cpp 
- Libraries
- Binance API used

## Architecture
Fetch - parse into json - function run this as parameters - outout shown

## Algorithms Used
| Algorithm | Problem | Complexity |
|---|---|---|
| Fenwick Tree | Order Book | O(log N) |
| Bellman-Ford | Arbitrage | O(VE) |
| DP Execution | Trade Scheduling | O(T·I) |

## Build Instructions

### Prerequisites
- Windows OS
- MinGW GCC compiler (with C++17 support)
- Git

---

### Option 1 — Code::Blocks (Recommended)
1. Clone the repository
```bash
   git clone https://github.com/YOURUSERNAME/AlgoTrade-X.git
```
2. Open `AlgoTrade-X.cbp` in Code::Blocks
3. Build → Debug → Build and Run (F9)

> Linker settings for `wininet` and `ws2_32` are pre-configured in the `.cbp` file.

---

### Option 2 — CMake (VS Code / Terminal)
1. Clone the repository
```bash
   git clone https://github.com/YOURUSERNAME/AlgoTrade-X.git
   cd AlgoTrade-X
```
2. Build
```bash
   mkdir build
   cd build
   cmake .. -G "MinGW Makefiles"
   mingw32-make
```
3. Run
```bash
   ./bin/AlgoTradeX.exe
```

> Requires CMake 3.10+ and MinGW installed and added to PATH.

---

### Note
This project uses Windows-only libraries (`wininet`, `ws2_32`) for HTTPS requests to the Binance API. Linux/Mac builds are not supported currently.

## Sample Output
(will paste)

## Results
(Benchmark table) - will paste

## Future Work
Need to add historical analysis of data.
Real algorithms.
Research and implement faster api calls.

## Author
Abhinav Pandey, JIIT, Noida, India
