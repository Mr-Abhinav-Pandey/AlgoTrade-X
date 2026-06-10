#ifndef REALTIME_CONFIG_H
#define REALTIME_CONFIG_H

#include <string>
#include "realtime_interfaces.h"

namespace realtime {

struct RealtimeConfig {
    WebSocketClientConfig websocket;
    StreamManagerConfig stream;
    MarketDataCacheConfig cache;

    bool enable_realtime_mode = false;
    int scan_interval_ms = 10000;
    std::string venue_name = "Binance";
    std::string app_name = "AlgoTradeX";
};

} // namespace realtime

#endif // REALTIME_CONFIG_H
