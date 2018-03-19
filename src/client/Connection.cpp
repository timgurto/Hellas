#include "Client.h"
#include "Connection.h"

void Connection::getNewMessages(std::queue<std::string> &messages) {
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = { 0, 10000 };
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        Client::instance().showErrorMessage("Error polling sockets: "s +
            toString(WSAGetLastError()), Color::FAILURE);
        return;
    }
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        const auto BUFFER_SIZE = 1023;
        static char buffer[BUFFER_SIZE + 1];
        int charsRead = recv(_socket.getRaw(), buffer, BUFFER_SIZE, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0) {
            buffer[charsRead] = '\0';
            messages.push(std::string(buffer));
        }
    }
}
