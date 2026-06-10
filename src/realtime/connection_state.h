#ifndef REALTIME_CONNECTION_STATE_H
#define REALTIME_CONNECTION_STATE_H

namespace realtime {

enum class ConnectionState {
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    RECONNECTING,
    SHUTDOWN
};

} // namespace realtime

#endif // REALTIME_CONNECTION_STATE_H
