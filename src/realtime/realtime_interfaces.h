#ifndef REALTIME_INTERFACES_H
#define REALTIME_INTERFACES_H

#include <functional>
#include <string>
#include <vector>
#include "market_snapshot.h"
#include "connection_state.h"

namespace realtime {

struct WebSocketClientConfig {
    std::string uri;
    std::vector<std::string> initial_streams;
    int reconnect_attempts = 5;
    int reconnect_backoff_ms = 1000;
    bool auto_reconnect = true;
    bool verbose_logging = false;
    bool verify_peer = true;
};

struct StreamManagerConfig {
    std::vector<std::string> symbols;
    int depth = 20;
    int update_interval_ms = 1000;
    bool use_aggregate_stream = true;
};

struct MarketDataCacheConfig {
    size_t max_symbols = 128;
    bool keep_snapshot_history = false;
    int history_depth = 10;
};

class IWebSocketClient {
public:
    virtual ~IWebSocketClient() = default;

    virtual void configure(const WebSocketClientConfig& config) = 0;
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    virtual void send(const std::string& message) = 0;
    virtual void set_message_handler(std::function<void(const std::string&)> handler) = 0;
    virtual void subscribe(const std::string& stream) = 0;
    virtual ConnectionState get_connection_state() const = 0;
};

class IStreamManager {
public:
    virtual ~IStreamManager() = default;

    virtual void configure(const StreamManagerConfig& config) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
    virtual void register_update_callback(std::function<void(const std::string& symbol, const std::string& payload)> callback) = 0;
};

class IMarketDataCache {
public:
    virtual ~IMarketDataCache() = default;

    virtual void configure(const MarketDataCacheConfig& config) = 0;
    virtual void update_snapshot(const std::string& symbol, const MarketSnapshot& snapshot) = 0;
    virtual bool get_latest_snapshot(const std::string& symbol, MarketSnapshot& out_snapshot) const = 0;
    virtual void clear() = 0;
};

} // namespace realtime

#endif // REALTIME_INTERFACES_H
