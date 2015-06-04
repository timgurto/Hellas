#include <cassert>
#include <sstream>

#include "Server.h"
#include "ServerMessage.h"

unsigned ServerMessage::_currentSerial = 0;
const Socket *ServerMessage::serverSocket = 0;

ServerMessage::ServerMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args):
_serial(_currentSerial++),
_timeSent(SDL_GetTicks()),
_dstSocket(dstSocket),
_msgCode(msgCode),
_args(args){
    // Compile message
    std::ostringstream oss;
    oss << *this;

    // Send message
    assert(serverSocket);
    serverSocket->sendMessage(oss.str(), _dstSocket);
}

ServerMessage::ServerMessage(unsigned serial):
_serial(serial){}

bool ServerMessage::operator<(const ServerMessage &rhs) const{
    return _serial < rhs._serial;
}

std::ostream &operator<<(std::ostream &lhs, const ServerMessage &rhs){
    lhs << '[' << rhs._serial << ',' << rhs._msgCode << ',' << rhs._args << ']';
    return lhs;
}

bool ServerMessage::expired(){
    return SDL_GetTicks() > _timeSent + Server::ACK_TIMEOUT;
}

ServerMessage ServerMessage::resend() const{
    ServerMessage s(_dstSocket, _msgCode, _args);
    return s;
}

bool ServerMessage::socketMatches(const Socket &rhs) const{
    return _dstSocket == rhs;
};

Uint32 ServerMessage::getLatency() const{
    return (SDL_GetTicks() - _timeSent) /2;
}
