#include "Socket.h"

int Socket::sockAddrSize = sizeof(sockaddr_in);
bool Socket::_winsockInitialized = false;
WSADATA Socket::_wsa;
std::map<SOCKET, int> Socket::_refCounts;

Socket::Socket(Log *debugLog):
_debug(debugLog){
    if (!_winsockInitialized)
        initWinsock();

    _raw = socket(AF_INET, SOCK_STREAM, 0);
    _refCounts[_raw] = 1;
}

Socket::Socket(SOCKET &rawSocket):
_raw(rawSocket),
_debug(0){
    _refCounts[_raw] = 1;
    rawSocket = INVALID_SOCKET;
}

Socket::Socket(const Socket &rhs):
_raw(rhs._raw),
_debug(rhs._debug){
    ++_refCounts[_raw];
}

const Socket &Socket::operator=(const Socket &rhs){
    _raw = rhs._raw;
    ++_refCounts[_raw];
    _debug = rhs._debug;
    return *this;
}

Socket::~Socket(){
    if (valid()) {
        --_refCounts[_raw];
        if (_refCounts[_raw] == 0) {
            closesocket(_raw);
            _refCounts.erase(_raw);
            if (_refCounts.empty())
                WSACleanup();
        }
    }
}

bool Socket::operator==(const Socket &rhs) const{
    return _raw == rhs._raw;
}

bool Socket::operator!=(const Socket &rhs) const{
    return !(*this == rhs);
}

bool Socket::operator<(const Socket &rhs) const{
    return _raw < rhs._raw;
}

void Socket::bind(sockaddr_in &socketAddr){
    if (!valid())
        return;

    if (::bind(_raw, (sockaddr*)&socketAddr, sockAddrSize) == SOCKET_ERROR) {
        if (_debug)
            (*_debug) << Color::RED << "Error binding socket: " << WSAGetLastError() <<  Log::endl;
    } else {
        if (_debug)
            (*_debug) << "Socket bound. " <<  Log::endl;
    }
}

void Socket::listen(){
    if (!valid())
        return;

    ::listen(_raw, 3);
}

SOCKET Socket::getRaw() const{
    return _raw;
}

void Socket::sendMessage(const std::string &msg, const Socket &destSocket) const{
    if (!_winsockInitialized)
        return;

    if (send(destSocket.getRaw(), msg.c_str(), (int)msg.length(), 0) < 0)
        if (_debug)
            (*_debug) << Color::RED << "Failed to send command \"" << msg << "\" to socket " << destSocket.getRaw() << Log::endl;
}

void Socket::sendMessage(const std::string &msg) const{
    sendMessage(msg, *this);
}

bool Socket::valid() const{
    return _raw != INVALID_SOCKET;
}

void Socket::initWinsock(){
    if (WSAStartup(MAKEWORD(2, 2), &_wsa) == 0)
        _winsockInitialized = true;
    
}
