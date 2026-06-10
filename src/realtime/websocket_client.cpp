#include "websocket_client.h"
#ifdef BUILD_WITH_BOOST_BEAST
#include "websocket_beast_client.h"
#endif
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <sstream>

namespace realtime {

struct BinanceWebSocketClient::Impl {
    WebSocketClientConfig config;
    std::function<void(const std::string&)> msg_handler;
    std::atomic<ConnectionState> state{ConnectionState::DISCONNECTED};
    std::thread worker;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stop_flag{false};
    std::vector<std::string> subscriptions;

    Impl() {}
    ~Impl() { stop(); }

    void start_worker() {
        stop_flag = false;
        worker = std::thread([this]{ run_loop(); });
    }

    void stop() {
        stop_flag = true;
        cv.notify_all();
        if (worker.joinable()) worker.join();
        state = ConnectionState::SHUTDOWN;
    }

    void run_loop() {
        int attempt = 0;
        while (!stop_flag) {
            state = ConnectionState::CONNECTING;
            if (config.verbose_logging) std::cerr << "[ws] connecting to " << config.uri << "\n";
            // Simulate connect success
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            state = ConnectionState::CONNECTED;
            attempt = 0;
            if (config.verbose_logging) std::cerr << "[ws] connected" << "\n";

            // Message pump: produce simulated messages for each subscription
            while (!stop_flag && state == ConnectionState::CONNECTED) {
                if (!subscriptions.empty() && msg_handler) {
                    for (const auto& s : subscriptions) {
                        std::ostringstream oss;
                        // emit simple bookTicker-like message
                        oss << "{\"e\":\"bookTicker\",\"s\":\"" << s << "\",\"b\":\"" << 1000.0 << "\",\"a\":\"" << 1001.0 << "\",\"E\":1234567890}";
                        try { msg_handler(oss.str()); } catch (...) {}
                        if (stop_flag) break;
                    }
                }
                // Wait or early exit
                std::unique_lock<std::mutex> lk(mtx);
                cv.wait_for(lk, std::chrono::milliseconds(1000));
            }

            if (stop_flag) break;

            // Simulate disconnect and attempt reconnects
            state = ConnectionState::RECONNECTING;
            if (config.verbose_logging) std::cerr << "[ws] disconnected, reconnecting..." << "\n";
            ++attempt;
            if (attempt > config.reconnect_attempts) {
                if (config.verbose_logging) std::cerr << "[ws] max reconnect attempts reached" << "\n";
                state = ConnectionState::DISCONNECTED;
                break;
            }
            int backoff = config.reconnect_backoff_ms * (1 << (attempt - 1));
            backoff = std::min(backoff, 30000);
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
        }
        if (config.verbose_logging) std::cerr << "[ws] run loop exiting" << "\n";
    }
};

BinanceWebSocketClient::BinanceWebSocketClient() : impl_(new Impl()) {}
BinanceWebSocketClient::~BinanceWebSocketClient() { impl_.reset(); }

void BinanceWebSocketClient::configure(const WebSocketClientConfig& config) {
    impl_->config = config;
}

void BinanceWebSocketClient::connect() {
    if (impl_->state == ConnectionState::CONNECTED) return;
    impl_->start_worker();
}

void BinanceWebSocketClient::disconnect() {
    impl_->stop();
}

bool BinanceWebSocketClient::is_connected() const {
    return impl_->state == ConnectionState::CONNECTED;
}

void BinanceWebSocketClient::send(const std::string& message) {
    if (impl_->config.verbose_logging) std::cerr << "[ws send] " << message << "\n";
}

void BinanceWebSocketClient::subscribe(const std::string& stream) {
    impl_->subscriptions.push_back(stream);
    if (impl_->config.verbose_logging) std::cerr << "[ws] subscribed to " << stream << "\n";
}

void BinanceWebSocketClient::set_message_handler(std::function<void(const std::string&)> handler) {
    impl_->msg_handler = std::move(handler);
}

ConnectionState BinanceWebSocketClient::get_connection_state() const {
    return impl_->state.load();
}

std::unique_ptr<IWebSocketClient> create_binance_websocket_client() {
    // If built with Boost.Beast, prefer the real implementation
#ifdef BUILD_WITH_BOOST_BEAST
    try {
        return std::make_unique<WebSocketBeastClient>();
    } catch (...) {
        return std::make_unique<BinanceWebSocketClient>();
    }
#else
    return std::make_unique<BinanceWebSocketClient>();
#endif
}

} // namespace realtime
