#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// Wrapper class for Winsock's SOCKET.  Singleton.
class Socket {
public:
    static int sockAddrSize;

private:
    static WSADATA _wsa;
    static SOCKET _raw;
    static bool _initialized;
    static int _instances;

public:
    Socket();
    ~Socket();

    void bind(sockaddr_in &socketAddr);
    void listen();
    SOCKET raw();

    void sendCommand(std::string msg);

    static void Socket::sendMessage(SOCKET s, std::string msg);
};

#endif
