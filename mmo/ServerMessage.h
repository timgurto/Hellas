#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include <SDL.h>
#include <string>

#include "User.h"
#include "messageCodes.h"

// Represents a message sent by the server to a client, and provides related utilities
class ServerMessage{
public:
    ServerMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args = "");
    ServerMessage(unsigned serial); // For set::find()
    static const Socket *serverSocket;

    bool operator<(const ServerMessage &rhs) const;

private:
    static unsigned _currentSerial;

    unsigned _serial;
    Uint32 _timeSent;
    const Socket *_dstSocket;
    unsigned _msgCode;
    std::string _args;
};

#endif
