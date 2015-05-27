#include "Socket.h"

int Socket::sockAddrSize = sizeof(sockaddr_in);
bool Socket::_winsockInitialized = false;
WSADATA Socket::_wsa;

Socket::Socket(Log *debugLog){
    if (!_winsockInitialized) {
        int ret = WSAStartup(MAKEWORD(2, 2), &_wsa);
        if (ret != 0)
            return;
        _winsockInitialized = true;

        _raw = socket(AF_INET, SOCK_STREAM, 0);
        if (_raw == INVALID_SOCKET)
            return;

        _debug = debugLog;
    }
}

Socket::~Socket(){
    if (valid()) {
        closesocket(_raw);
        WSACleanup();
    }
}

void Socket::bind(sockaddr_in &socketAddr){
    if (!valid())
        return;

    if (::bind(_raw, (sockaddr*)&socketAddr, sockAddrSize) == SOCKET_ERROR)
        if (_debug) (*_debug) << "Error binding socket: " << WSAGetLastError() <<  Log::endl;
    else
        if (_debug) (*_debug) << "Socket bound. " <<  Log::endl;
}

void Socket::listen(){
    if (!valid())
        return;

    ::listen(_raw, 3);
}

SOCKET Socket::raw(){
    if (valid())
        return _raw;
    return -1;
}

// Send a client command to the server
void Socket::sendCommand(std::string msg) {
    if (!valid())
        return;

    if (send(_raw, msg.c_str(), (int)msg.length(), 0) < 0)
        if (_debug) (*_debug) << "Failed to send command: " << msg << Log::endl;
    else
        if (_debug) (*_debug) << "Sent command: " << msg << Log::endl;
}

void Socket::sendMessage(SOCKET s, std::string msg, Log *debug){
    if (!_winsockInitialized)
        return;

    if (send(s, msg.c_str(), (int)msg.length(), 0) < 0)
        if (debug) (*debug) << "Failed to send command: " << msg << Log::endl;
    else
        if (debug) (*debug) << "Sent command: " << msg << Log::endl;
}

bool Socket::valid() const{
    return _raw != INVALID_SOCKET;
}
