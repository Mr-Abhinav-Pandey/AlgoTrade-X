#ifndef MARKET_SNAPSHOT_H
#define MARKET_SNAPSHOT_H

#include <cstdint>
#include <utility>
#include <vector>

// One frozen view of market data for a single live scan.
// Populated by build_market_snapshot_rest() in main.cpp (REST today).
struct MarketSnapshot {
    int64_t assembled_ms = 0;

    // Mid tickers (theoretical arbitrage layer).
    double btc_mid = 0;
    double btcusdt = 0;
    double ethusdt = 0;
    double bnbusdt = 0;
    double ethbtc = 0;
    double bnbbtc = 0;

    // Best bid / ask per pair (executable arbitrage layer).
    double btcusdt_bid = 0;
    double btcusdt_ask = 0;
    double ethusdt_bid = 0;
    double ethusdt_ask = 0;
    double bnbusdt_bid = 0;
    double bnbusdt_ask = 0;
    double ethbtc_bid = 0;
    double ethbtc_ask = 0;
    double bnbbtc_bid = 0;
    double bnbbtc_ask = 0;

    // True only when every pair has valid bid < ask (see builder in main.cpp).
    bool executable_ready = false;

    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;

    bool is_complete() const {
        return btc_mid > 0 && btcusdt > 0 && ethusdt > 0 && bnbusdt > 0
            && ethbtc > 0 && bnbbtc > 0 && !bids.empty() && !asks.empty();
    }
};

#endif
