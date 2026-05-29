#ifndef MARKET_SNAPSHOT_H
#define MARKET_SNAPSHOT_H

#include <cstdint>
#include <utility>
#include <vector>

// One frozen view of market data for a single live scan.
// Populated by build_market_snapshot_rest() in main.cpp (REST today).
struct MarketSnapshot {
    int64_t assembled_ms = 0;

    // BTCUSDT reference price (book mid and FX leg share this value).
    double btc_mid = 0;
    double btcusdt = 0;

    double ethusdt = 0;
    double bnbusdt = 0;
    double ethbtc = 0;
    double bnbbtc = 0;

    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;

    bool is_complete() const {
        return btc_mid > 0 && btcusdt > 0 && ethusdt > 0 && bnbusdt > 0
            && ethbtc > 0 && bnbbtc > 0 && !bids.empty() && !asks.empty();
    }
};

#endif
