#include <mutex>

#include "Socket.h"

Log *Socket::debug = nullptr;

int Socket::sockAddrSize = sizeof(sockaddr_in);
bool Socket::_winsockInitialized = false;
WSADATA Socket::_wsa;
std::map<SOCKET, int> Socket::_refCounts;

Socket::Socket():
_lingerTime(0){
    if (!_winsockInitialized)
        initWinsock();

    _raw = socket(AF_INET, SOCK_STREAM, 0);
    addRef();
}

Socket::Socket(SOCKET &rawSocket):
_raw(rawSocket),
_lingerTime(0){
    addRef();
    rawSocket = INVALID_SOCKET;
}

Socket::Socket(const Socket &rhs):
_raw(rhs._raw),
_lingerTime(rhs._lingerTime){
    if (_raw == INVALID_SOCKET)
        return;
    addRef();
}

Socket Socket::Empty(){
    SOCKET invalidSocket = INVALID_SOCKET;
    return Socket(invalidSocket);
}

Socket::~Socket(){
    close();
}

const Socket &Socket::operator=(const Socket &rhs){
    if (this == &rhs)
        return *this;

    close();

    _raw = rhs._raw;
    addRef();
    _lingerTime = rhs._lingerTime;
    return *this;
}

void Socket::addRef(){
    if (_refCounts.find(_raw) != _refCounts.end())
        ++_refCounts[_raw];
    else
        _refCounts[_raw] = 1;
}

void Socket::bind(sockaddr_in &socketAddr){
    if (!valid())
        return;

    if (::bind(_raw, (sockaddr*)&socketAddr, sockAddrSize) == SOCKET_ERROR) {
        *debug << Color::RED << "Error binding socket: " << WSAGetLastError() <<  Log::endl;
    } else {
        *debug << "Socket bound. " <<  Log::endl;
    }
}

void Socket::listen(){
    if (!valid())
        return;

    ::listen(_raw, 3);
}

void Socket::sendMessage(const std::string &msg, const Socket &destSocket) const{
    if (!_winsockInitialized)
        return;

    static std::mutex mutex;
    mutex.lock();

    if (send(destSocket.getRaw(), msg.c_str(), (int)msg.length(), 0) < 0) {
        if (debug)
            *debug << Color::RED << "Failed to send command \"" << msg
                   << "\" to socket " << destSocket.getRaw() << Log::endl;
    }

    mutex.unlock();
}

void Socket::sendMessage(const std::string &msg) const{
    sendMessage(msg, *this);
}

void Socket::initWinsock(){
    if (WSAStartup(MAKEWORD(2, 2), &_wsa) == 0)
        _winsockInitialized = true;
    
}

void Socket::delayClosing(ms_t lingerTime){
    _lingerTime = lingerTime;
}

int Socket::closeRawAfterDelay(void *data){
    ms_t *const p = static_cast<ms_t *>(data);
    const SOCKET s = static_cast<SOCKET>(p[0]);
    const ms_t delay = p[1];
    delete[] p;

    SDL_Delay(delay);
    closesocket(s);

    return 0;
}

void Socket::close(){
    if (valid()) {
        --_refCounts[_raw];
        if (_refCounts[_raw] == 0) {
            if (_lingerTime > 0) {
                ms_t *const args = new ms_t[2];
                args[0] = static_cast<ms_t>(_raw);
                args[1] = _lingerTime;
                SDL_CreateThread(closeRawAfterDelay,
                                 "Closing socket",
                                 static_cast<void*>(args));
            } else {
                closesocket(_raw);
            }
            _refCounts.erase(_raw);
            if (_refCounts.empty()){
                WSACleanup();
                _winsockInitialized = false;
            }
        }
    }
}
