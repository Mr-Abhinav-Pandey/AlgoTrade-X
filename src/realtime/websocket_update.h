#ifndef REALTIME_WEBSOCKET_UPDATE_H
#define REALTIME_WEBSOCKET_UPDATE_H

#include <cstdint>
#include <string>
#include <vector>

namespace realtime {

enum class BinanceUpdateType {
    UNKNOWN,
    TICKER,
    DEPTH,
    SUBSCRIPTION_RESPONSE
};

struct RawTickerUpdate {
    std::string stream;
    std::string symbol;
    double bid_price = 0.0;
    double ask_price = 0.0;
    int64_t event_time = 0;
};

struct RawDepthUpdate {
    std::string stream;
    std::string symbol;
    int64_t event_time = 0;
    int64_t update_id = 0;
    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;
};

struct BinanceStreamEvent {
    BinanceUpdateType type = BinanceUpdateType::UNKNOWN;
    RawTickerUpdate ticker;
    RawDepthUpdate depth;
    std::string debug_message;
};

} // namespace realtime

#endif // REALTIME_WEBSOCKET_UPDATE_H
