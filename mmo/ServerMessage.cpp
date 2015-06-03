#include <cassert>
#include <sstream>

#include "ServerMessage.h"

unsigned ServerMessage::_currentSerial = 0;
 const Socket *ServerMessage::serverSocket = 0;

ServerMessage::ServerMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args):
_serial(_currentSerial++),
_timeSent(SDL_GetTicks()),
_dstSocket(&dstSocket),
_msgCode(msgCode),
_args(args){
    // Compile message
    std::ostringstream oss;
    oss << '[' << _serial << ',' << _msgCode << ',' << _args << ']';

    // Send message
    assert(serverSocket);
    serverSocket->sendMessage(oss.str(), *_dstSocket);
}

ServerMessage::ServerMessage(unsigned serial):
_serial(serial){}

bool ServerMessage::operator<(const ServerMessage &rhs) const{
    return _serial < rhs._serial;
}
