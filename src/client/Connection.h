#pragma once

#include <queue>

#include "../Socket.h"

// Manages a connection to the server
class Connection {
public:
    void getNewMessages(std::queue<std::string> &messages);
    const Socket &socket() const { return _socket; } // TODO: remove
    void clearSocket() { _socket = {}; } // TODO: remove

private:
    Socket _socket;

};
