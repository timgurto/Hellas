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
    static Log *debug;

private:
    static WSADATA _wsa;
    static bool _winsockInitialized;
    SOCKET _raw;
    Uint32 _lingerTime;
    static std::map<SOCKET, int> _refCounts; // Reference counters for each raw SOCKET

    static void initWinsock();
    void addRef(); // Increment reference counter
    static int closeRawAfterDelay(void *data); // Thread function
    void close(); // Decrement reference counter, and close socket if no references remain

public:
    Socket(const Socket &rhs);
    Socket();
    Socket(SOCKET &rawSocket);
    const Socket &operator=(const Socket &rhs);
    ~Socket();

    bool operator==(const Socket &rhs) const;
    bool operator!=(const Socket &rhs) const;
    bool operator<(const Socket &rhs) const;

    void bind(sockaddr_in &socketAddr);
    void listen();
    bool valid() const; // Whether this socket is safe to use
    SOCKET getRaw() const;
    void delayClosing(Uint32 lingerTime); // Delay closing of socket

    // No destination socket implies client->server message,
    // Server -> client messages should use the ServerMessage class, rather than calling sendMessage() directly.
    void Socket::sendMessage(const std::string &msg) const;
    void Socket::sendMessage(const std::string &msg, const Socket &destSocket) const;
};

#endif
