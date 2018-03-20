#pragma once

#include <queue>

#include "../Socket.h"

class Client;

// Manages a connection to the server
class Connection {

public:
    enum State { // TODO: make private
        INITIALIZING,      // }
        TRYING_TO_CONNECT, // } Login screen
        CONNECTED,         // }
        LOGGED_IN,
        LOADED,
        CONNECTION_ERROR
    };

    Connection(Client &client);

    void getNewMessages();
    void connect();

    const Socket &socket() const { return _socket; } // TODO: remove
    void clearSocket() { _socket = {}; } // TODO: remove
    void state(State s) { _state = s; } // TODO: remove
    State state() const { return _state; } // TODO: remove
    bool isAThreadConnecting() const { return _aThreadIsConnecting; } // TODO: remove
    void aThreadIsConnecting() { _aThreadIsConnecting = true; } // TODO: remove

    void showError(const std::string &msg) const;

private:
    Socket _socket;
    State _state{ INITIALIZING };
    Client &_client;

    ms_t _timeOfLastConnectionAttempt{ 0 };

    bool _aThreadIsConnecting{ false };

    static const ms_t TIME_BETWEEN_CONNECTION_ATTEMPTS{ 3000 };
    static const u_short DEBUG_PORT{ 8888 };
    static const u_short PRODUCTION_PORT{ 8889 };
};
