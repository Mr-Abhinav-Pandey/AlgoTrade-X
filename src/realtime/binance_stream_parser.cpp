#include "binance_stream_parser.h"
#include <nlohmann/json.hpp>

namespace realtime {

using json = nlohmann::json;

BinanceStreamEvent BinanceStreamParser::parse(const std::string& raw_message) const {
    BinanceStreamEvent out;
    if (raw_message.empty()) {
        out.debug_message = "empty message";
        return out;
    }

    try {
        auto doc = json::parse(raw_message);
        if (!doc.is_object()) {
            out.debug_message = "parsed JSON is not an object";
            return out;
        }

        std::string outer_stream;
        if (doc.contains("stream") && doc["stream"].is_string()) {
            outer_stream = doc["stream"].get<std::string>();
        }
        if (doc.contains("data") && doc["data"].is_object()) {
            doc = doc["data"];
        }

        if (doc.contains("e") && doc["e"].is_string()) {
            std::string event = doc["e"].get<std::string>();
            if (event == "bookTicker" || event == "24hrTicker") {
                out.type = BinanceUpdateType::TICKER;
                out.ticker.stream = doc.value("s", outer_stream);
                out.ticker.symbol = out.ticker.stream;
                out.ticker.bid_price = doc.value("b", 0.0);
                out.ticker.ask_price = doc.value("a", 0.0);
                out.ticker.last_price = doc.value("c", 0.0);
                out.ticker.event_time = doc.value("E", int64_t(0));
                return out;
            }
            if (event == "depthUpdate") {
                out.type = BinanceUpdateType::DEPTH;
                out.depth.stream = doc.value("s", outer_stream);
                out.depth.symbol = out.depth.stream;
                out.depth.update_id = doc.value("u", int64_t(0));
                out.depth.event_time = doc.value("E", int64_t(0));

                if (doc.contains("b")) {
                    for (const auto& entry : doc["b"]) {
                        if (entry.is_array() && entry.size() >= 2) {
                            out.depth.bids.emplace_back(std::stod(entry[0].get<std::string>()), std::stod(entry[1].get<std::string>()));
                        }
                    }
                }
                if (doc.contains("a")) {
                    for (const auto& entry : doc["a"]) {
                        if (entry.is_array() && entry.size() >= 2) {
                            out.depth.asks.emplace_back(std::stod(entry[0].get<std::string>()), std::stod(entry[1].get<std::string>()));
                        }
                    }
                }
                return out;
            }
        }

        if (doc.contains("result")) {
            out.type = BinanceUpdateType::SUBSCRIPTION_RESPONSE;
            out.debug_message = raw_message;
            return out;
        }

        out.debug_message = "unrecognized event type";
    } catch (const std::exception& ex) {
        out.debug_message = std::string("JSON parse failed: ") + ex.what();
    }
    return out;
}

} // namespace realtime
