#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <windows.h>

#include "Log.h"

#pragma comment(lib, "ws2_32.lib")

// Wrapper class for Winsock's SOCKET.
class Socket {
public:
    static int sockAddrSize;

private:
    static WSADATA _wsa;
    static bool _winsockInitialized;
    SOCKET _raw;
    Log *_debug;

public:
    Socket(Log *debugLog = 0);
    ~Socket();

    void bind(sockaddr_in &socketAddr);
    void listen();
    bool valid() const; // Whether this socket is safe to use
    SOCKET getRaw() const;

    // Default socket implies client->server message
    void Socket::sendMessage(const std::string &msg, SOCKET destSocket = INVALID_SOCKET) const;
};

#endif
