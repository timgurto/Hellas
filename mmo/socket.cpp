#include "Socket.h"

int Socket::sockAddrSize = sizeof(sockaddr_in);
int Socket::_instances = 0;
bool Socket::_initialized = false;
WSADATA Socket::_wsa;
SOCKET Socket::_raw;

Socket::Socket(){
    ++ _instances;

    if (!_initialized) {
        int ret = WSAStartup(MAKEWORD(2, 2), &_wsa);
        if (ret != 0)
            return;

        _raw = socket(AF_INET, SOCK_STREAM, 0);
        if (_raw == INVALID_SOCKET)
            return;

        _initialized = true;
    }
}

Socket::~Socket(){
    -- _instances;

    if (_initialized) {
        closesocket(_raw);
        WSACleanup();
    }
}

void Socket::bind(sockaddr_in &socketAddr){
    if (!_initialized)
        return;

    if (::bind(_raw, (sockaddr*)&socketAddr, sockAddrSize) == SOCKET_ERROR)
        std::cout << "Error binding socket: " << WSAGetLastError() <<  std::endl;
    else
        std::cout << "Socket bound. " <<  std::endl;
}

void Socket::listen(){
    if (!_initialized)
        return;

    ::listen(_raw, 3);
}

SOCKET Socket::raw(){
    if (!_initialized)
        return 0;

    return _raw;
}

// Send a client command to the server
void Socket::sendCommand(std::string msg) {
    if (!_initialized)
        return;

    if (send(_raw, msg.c_str(), (int)msg.length(), 0) < 0)
        std::cout << "Failed to send command: " << msg << std::endl;
    else
        std::cout << "Sent command: " << msg << std::endl;
}

void Socket::sendMessage(SOCKET s, std::string msg){
    if (!_initialized)
        return;

    if (send(s, msg.c_str(), (int)msg.length(), 0) < 0)
        std::cout << "Failed to send command: " << msg << std::endl;
    else
        std::cout << "Sent command: " << msg << std::endl;
}
