#ifndef REALTIME_BINANCE_STREAM_PARSER_H
#define REALTIME_BINANCE_STREAM_PARSER_H

#include "websocket_update.h"
#include <string>

namespace realtime {

class BinanceStreamParser {
public:
    BinanceStreamEvent parse(const std::string& raw_message) const;
};

} // namespace realtime

#endif // REALTIME_BINANCE_STREAM_PARSER_H
