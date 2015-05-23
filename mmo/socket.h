#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// Wrapper class for WInsock's SOCKET
class Socket {
public:
    static int sockAddrSize;

private:
    SOCKET _raw;

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
