#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include <iostream>
#include <SDL.h>
#include <string>

#include "User.h"
#include "messageCodes.h"

// Represents a message sent by the server to a client, and provides related utilities
class ServerMessage{
public:
    ServerMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args = "");
    static const Socket *serverSocket;

    friend std::ostream &operator<<(std::ostream &lhs, const ServerMessage &rhs);

private:
    Socket _dstSocket;
    MessageCode _msgCode;
    std::string _args;
};

#endif
