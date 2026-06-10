#ifndef REALTIME_WEBSOCKET_CLIENT_H
#define REALTIME_WEBSOCKET_CLIENT_H

#include "realtime_interfaces.h"
#include "connection_state.h"
#include <functional>
#include <memory>
#include <string>

namespace realtime {

std::unique_ptr<IWebSocketClient> create_binance_websocket_client();

class BinanceWebSocketClient : public IWebSocketClient {
public:
    BinanceWebSocketClient();
    ~BinanceWebSocketClient() override;

    void configure(const WebSocketClientConfig& config) override;
    void connect() override;
    void disconnect() override;
    bool is_connected() const override;
    void send(const std::string& message) override;
    void subscribe(const std::string& stream) override;
    void set_message_handler(std::function<void(const std::string&)> handler) override;
    ConnectionState get_connection_state() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace realtime

#endif // REALTIME_WEBSOCKET_CLIENT_H
