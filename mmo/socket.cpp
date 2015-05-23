#include "Socket.h"

int Socket::sockAddrSize = sizeof(sockaddr_in);

Socket::Socket(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        return;
    }
    std::cout << "Winsock initialized" << std::endl;

    _raw = socket(AF_INET, SOCK_STREAM, 0);
    if (_raw == INVALID_SOCKET) {
        std::cout << "Could not create socket: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "Socket created" << std::endl;
}

Socket::~Socket(){
    closesocket(_raw);
    WSACleanup();
}

void Socket::bind(sockaddr_in &socketAddr){
    if (::bind(_raw, (sockaddr*)&socketAddr, sockAddrSize) == SOCKET_ERROR)
        std::cout << "Error binding socket: " << WSAGetLastError() <<  std::endl;
    else
        std::cout << "Socket bound. " <<  std::endl;
}

void Socket::listen(){
    ::listen(_raw, 3);
}

SOCKET Socket::raw(){
    return _raw;
}

// Send a client command to the server
void Socket::sendCommand(std::string msg) {
    if (send(_raw, msg.c_str(), (int)msg.length(), 0) < 0)
        std::cout << "Failed to send command: " << msg << std::endl;
    else
        std::cout << "Sent command: " << msg << std::endl;
}

void Socket::sendMessage(SOCKET s, std::string msg){
    if (send(s, msg.c_str(), (int)msg.length(), 0) < 0)
        std::cout << "Failed to send command: " << msg << std::endl;
    else
        std::cout << "Sent command: " << msg << std::endl;
}
