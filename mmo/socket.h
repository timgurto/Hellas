#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <map>
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
    static std::map<SOCKET, int> _refCounts; // Reference counters for each raw SOCKET

    static void initWinsock();

public:
    Socket(const Socket &rhs);
    Socket(Log *debugLog = 0);
    Socket(SOCKET &rawSocket);
    const Socket &operator=(const Socket &rhs);
    ~Socket();

    void bind(sockaddr_in &socketAddr);
    void listen();
    bool valid() const; // Whether this socket is safe to use
    SOCKET getRaw() const;

    // No destination socket implies client->server message
    void Socket::sendMessage(const std::string &msg) const;
    void Socket::sendMessage(const std::string &msg, const Socket &destSocket) const;
};

#endif
