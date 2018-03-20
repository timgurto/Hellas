#pragma once

#include <queue>

#include "../Socket.h"

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

    void getNewMessages(std::queue<std::string> &messages);
    const Socket &socket() const { return _socket; } // TODO: remove
    void clearSocket() { _socket = {}; } // TODO: remove
    void state(State s) { _state = s; } // TODO: remove
    State state() const { return _state; } // TODO: remove

private:
    Socket _socket;
    State _state{ INITIALIZING };
};
