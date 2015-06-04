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
    ServerMessage(unsigned serial); // For set::find()
    static const Socket *serverSocket;

    bool operator<(const ServerMessage &rhs) const;

    bool expired();
    ServerMessage resend() const; // Return a new ServerMessage with the same payload

    friend std::ostream &operator<<(std::ostream &lhs, const ServerMessage &rhs);

    bool socketMatches(const Socket &rhs) const;

//private:
    static unsigned _currentSerial;

    unsigned _serial;
    Uint32 _timeSent;
    Socket _dstSocket;
    MessageCode _msgCode;
    std::string _args;
};

#endif
