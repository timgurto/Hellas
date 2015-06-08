#include <cassert>
#include <sstream>

#include "Server.h"
#include "ServerMessage.h"

const Socket *ServerMessage::serverSocket = 0;

ServerMessage::ServerMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args):
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

std::ostream &operator<<(std::ostream &lhs, const ServerMessage &rhs){
    lhs << '[' << rhs._msgCode;
    if (rhs._args != "")
        lhs << ',' << rhs._args;
    lhs << ']';
    return lhs;
}
