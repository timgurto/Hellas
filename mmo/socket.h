#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

class Socket {
public:
    static const int MAX_CLIENTS;
    static const int BUFFER_SIZE;
    static int sockAddrSize;

private:
    SOCKET s;

public:
    Socket();
    ~Socket();

    int runServer();

    int runClient();
    void sendCommand(std::string msg);
};

#endif
