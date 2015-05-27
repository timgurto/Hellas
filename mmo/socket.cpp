#include "Socket.h"

int Socket::sockAddrSize = sizeof(sockaddr_in);
int Socket::_instances = 0;
bool Socket::_initialized = false;
WSADATA Socket::_wsa;
SOCKET Socket::_raw;
Log *Socket::_debug;

Socket::Socket(Log *debugLog){
    ++ _instances;

    if (!_initialized) {
        int ret = WSAStartup(MAKEWORD(2, 2), &_wsa);
        if (ret != 0)
            return;

        _raw = socket(AF_INET, SOCK_STREAM, 0);
        if (_raw == INVALID_SOCKET)
            return;

        _initialized = true;

        _debug = debugLog;
    }
}

Socket::~Socket(){
    -- _instances;

    if (_initialized && _instances == 0) {
        closesocket(_raw);
        WSACleanup();
    }
}

void Socket::bind(sockaddr_in &socketAddr){
    if (!_initialized)
        return;

    if (::bind(_raw, (sockaddr*)&socketAddr, sockAddrSize) == SOCKET_ERROR)
        if (_debug) (*_debug) << "Error binding socket: " << WSAGetLastError() <<  Log::endl;
    else
        if (_debug) (*_debug) << "Socket bound. " <<  Log::endl;
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
        if (_debug) (*_debug) << "Failed to send command: " << msg << Log::endl;
    else
        if (_debug) (*_debug) << "Sent command: " << msg << Log::endl;
}

void Socket::sendMessage(SOCKET s, std::string msg){
    if (!_initialized)
        return;

    if (send(s, msg.c_str(), (int)msg.length(), 0) < 0)
        if (_debug) (*_debug) << "Failed to send command: " << msg << Log::endl;
    else
        if (_debug) (*_debug) << "Sent command: " << msg << Log::endl;
}
