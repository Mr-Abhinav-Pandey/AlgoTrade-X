#ifdef BUILD_WITH_BOOST_BEAST

#include "websocket_beast_client.h"
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <thread>
#include <iostream>

namespace realtime {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

struct WebSocketBeastClient::Impl {
    WebSocketClientConfig config;
    std::function<void(const std::string&)> msg_handler;
    std::atomic<ConnectionState> state{ConnectionState::DISCONNECTED};

    net::io_context ioc;
    net::ssl::context ctx{net::ssl::context::tlsv12_client};
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws;
    std::thread ioc_thread;
    std::mutex mtx;
    std::vector<std::string> subscriptions;

    Impl() {
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(net::ssl::verify_peer);
    }

    ~Impl() {
        stop();
    }

    void run_io() {
        ioc.run();
    }

    void stop() {
        try {
            if (ws && ws->is_open()) {
                beast::error_code ec;
                ws->close(websocket::close_code::normal, ec);
            }
        } catch (...) {}
        ioc.stop();
        if (ioc_thread.joinable()) ioc_thread.join();
        state = ConnectionState::SHUTDOWN;
    }

    void on_read(beast::flat_buffer& buffer) {
        try {
            std::string msg = beast::buffers_to_string(buffer.data());
            if (msg_handler) msg_handler(msg);
        } catch (...) {}
    }
};

WebSocketBeastClient::WebSocketBeastClient() : impl_(new Impl()) {}
WebSocketBeastClient::~WebSocketBeastClient() { impl_.reset(); }

void WebSocketBeastClient::configure(const WebSocketClientConfig& config) {
    impl_->config = config;
}

void WebSocketBeastClient::connect() {
    if (impl_->state == ConnectionState::CONNECTED) return;
    impl_->state = ConnectionState::CONNECTING;

    // Parse uri e.g. wss://stream.binance.com:9443/ws
    std::string uri = impl_->config.uri;
    // Basic parsing
    const std::string https_prefix = "wss://";
    if (uri.rfind(https_prefix, 0) != 0) {
        throw std::runtime_error("Only wss:// URIs supported");
    }
    std::string hostport = uri.substr(https_prefix.size());
    std::string host = hostport;
    std::string port = "443";
    auto pos = hostport.find('/');
    if (pos != std::string::npos) host = hostport.substr(0,pos);
    auto colon = host.find(':');
    if (colon != std::string::npos) {
        port = host.substr(colon+1);
        host = host.substr(0,colon);
    }

    // Resolve and connect
    tcp::resolver resolver(impl_->ioc);
    auto const results = resolver.resolve(host, port);
    impl_->ws = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(impl_->ioc, impl_->ctx);

    beast::get_lowest_layer(*impl_->ws).connect(results);
    impl_->ws->next_layer().handshake(net::ssl::stream_base::client);
    impl_->ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    impl_->ws->handshake(host, "/");

    impl_->state = ConnectionState::CONNECTED;

    // Start ioc thread
    impl_->ioc_thread = std::thread([this]{ impl_->run_io(); });

    // Start read loop asynchronously
    auto self = impl_.get();
    std::thread([self](){
        beast::flat_buffer buffer;
        try {
            while (self->ws && self->ws->is_open()) {
                beast::error_code ec;
                self->ws->read(buffer, ec);
                if (ec) {
                    if (self->config.verbose_logging) std::cerr << "[beast read ec] " << ec.message() << "\n";
                    break;
                }
                self->on_read(buffer);
                buffer.consume(buffer.size());
            }
        } catch (const std::exception& ex) {
            if (self->config.verbose_logging) std::cerr << "[beast read exception] " << ex.what() << "\n";
        }
        self->state = ConnectionState::DISCONNECTED;
    }).detach();
}

void WebSocketBeastClient::disconnect() {
    impl_->stop();
}

bool WebSocketBeastClient::is_connected() const {
    return impl_->state == ConnectionState::CONNECTED;
}

void WebSocketBeastClient::send(const std::string& message) {
    if (!impl_->ws) return;
    beast::error_code ec;
    impl_->ws->write(net::buffer(message), ec);
}

void WebSocketBeastClient::subscribe(const std::string& stream) {
    // Binance websocket uses JSON subscribe messages for combined streams when using websocket API
    // For simplicity, send a subscribe JSON via send(). Upstream may call subscribe multiple times.
    impl_->subscriptions.push_back(stream);
    json j;
    j["method"] = "SUBSCRIBE";
    j["params"] = json::array({stream});
    j["id"] = 1;
    send(j.dump());
}

void WebSocketBeastClient::set_message_handler(std::function<void(const std::string&)> handler) {
    impl_->msg_handler = std::move(handler);
}

ConnectionState WebSocketBeastClient::get_connection_state() const {
    return impl_->state.load();
}

} // namespace realtime

#endif // BUILD_WITH_BOOST_BEAST
