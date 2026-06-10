#include "binance_stream_parser.h"
#include <string>
#include <sstream>
#include <algorithm>

namespace realtime {

static std::string find_json_field(const std::string& s, const std::string& key) {
    std::string search = '"' + key + '"';
    auto pos = s.find(search);
    if (pos == std::string::npos) return "";
    auto colon = s.find(':', pos);
    if (colon == std::string::npos) return "";
    auto start = s.find_first_not_of(" \t\n\r", colon + 1);
    if (start == std::string::npos) return "";
    if (s[start] == '"') {
        auto end = s.find('"', start + 1);
        if (end == std::string::npos) return "";
        return s.substr(start + 1, end - start - 1);
    }
    // numeric or array
    auto end = s.find_first_of(",}\n\r", start + 1);
    if (end == std::string::npos) end = s.size();
    return s.substr(start, end - start);
}

BinanceStreamEvent BinanceStreamParser::parse(const std::string& raw_message) const {
    BinanceStreamEvent out;
    if (raw_message.empty()) {
        out.debug_message = "empty message";
        return out;
    }

    // Quick checks for common Binance event types.
    if (raw_message.find("\"e\":\"bookTicker\"") != std::string::npos || raw_message.find("\"u\"") != std::string::npos) {
        out.type = BinanceUpdateType::TICKER;
        try {
            out.ticker.stream = find_json_field(raw_message, "s");
            out.ticker.symbol = out.ticker.stream;
            std::string bp = find_json_field(raw_message, "b");
            std::string ap = find_json_field(raw_message, "a");
            if (!bp.empty()) out.ticker.bid_price = std::stod(bp);
            if (!ap.empty()) out.ticker.ask_price = std::stod(ap);
            std::string t = find_json_field(raw_message, "E");
            if (!t.empty()) out.ticker.event_time = std::stoll(t);
        } catch (...) {
            out.debug_message = "ticker parse error";
            out.type = BinanceUpdateType::UNKNOWN;
        }
        return out;
    }

    if (raw_message.find("\"e\":\"depthUpdate\"") != std::string::npos || raw_message.find("\"asks\"") != std::string::npos) {
        out.type = BinanceUpdateType::DEPTH;
        try {
            out.depth.stream = find_json_field(raw_message, "s");
            out.depth.symbol = out.depth.stream;
            std::string u = find_json_field(raw_message, "u");
            if (!u.empty()) out.depth.update_id = std::stoll(u);
            std::string et = find_json_field(raw_message, "E");
            if (!et.empty()) out.depth.event_time = std::stoll(et);
            // parse bids/asks arrays naive: look for "bids":[["price","qty"],...]
            auto parse_levels = [&](const std::string& key, std::vector<std::pair<double,double>>& out_levels){
                auto pos = raw_message.find('"' + key + '"');
                if (pos == std::string::npos) return;
                auto arr = raw_message.substr(pos);
                size_t p = arr.find('[');
                if (p == std::string::npos) return;
                int depth = 0;
                size_t i = p;
                bool in_pair=false;
                std::string num;
                std::vector<std::string> tokens;
                for (; i < arr.size(); ++i) {
                    char c = arr[i];
                    if (c == '[') { depth++; if (depth==2) in_pair=true; continue; }
                    if (c == ']') { depth--; if (depth<2) break; }
                    if (in_pair) {
                        if (c == '"') continue;
                        if (c == ',' || c == ']') {
                            if (!num.empty()) { tokens.push_back(num); num.clear(); }
                            if ((int)tokens.size() == 2) {
                                double p0 = stod(tokens[0]);
                                double p1 = stod(tokens[1]);
                                out_levels.emplace_back(p0,p1);
                                tokens.clear();
                                in_pair=false;
                            }
                            continue;
                        }
                        if (!isspace((unsigned char)c)) num.push_back(c);
                    }
                }
            };
            parse_levels("bids", out.depth.bids);
            parse_levels("asks", out.depth.asks);
        } catch (...) {
            out.debug_message = "depth parse error";
            out.type = BinanceUpdateType::UNKNOWN;
        }
        return out;
    }

    // subscription / response
    if (raw_message.find("\"result\":") != std::string::npos) {
        out.type = BinanceUpdateType::SUBSCRIPTION_RESPONSE;
        out.debug_message = raw_message;
        return out;
    }

    out.debug_message = "unrecognized message";
    return out;
}

} // namespace realtime
