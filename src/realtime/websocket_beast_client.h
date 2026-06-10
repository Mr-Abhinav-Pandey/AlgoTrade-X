#ifndef REALTIME_WEBSOCKET_BEAST_CLIENT_H
#define REALTIME_WEBSOCKET_BEAST_CLIENT_H

#ifdef BUILD_WITH_BOOST_BEAST

#include "websocket_client.h"
#include "websocket_update.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <functional>
#include <string>

namespace realtime {

class WebSocketBeastClient : public IWebSocketClient {
public:
    explicit WebSocketBeastClient();
    ~WebSocketBeastClient() override;

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

#endif // BUILD_WITH_BOOST_BEAST

#endif // REALTIME_WEBSOCKET_BEAST_CLIENT_H
