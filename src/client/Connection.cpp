#include "../Args.h"
#include "Client.h"
#include "Connection.h"

extern Args cmdLineArgs;

Connection::Connection(Client & client):
_client(client)
{}

void Connection::getNewMessages() {
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = { 0, 10000 };
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        showError("Error polling sockets: "s + toString(WSAGetLastError()));
        return;
    }
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        const auto BUFFER_SIZE = 1023;
        static char buffer[BUFFER_SIZE + 1];
        int charsRead = recv(_socket.getRaw(), buffer, BUFFER_SIZE, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0) {
            buffer[charsRead] = '\0';
            _client._messages.push(std::string(buffer));
        }
    }
}

void Connection::connect() {
    auto timeNow = SDL_GetTicks();
    if (timeNow - _timeOfLastConnectionAttempt < TIME_BETWEEN_CONNECTION_ATTEMPTS) {
        _aThreadIsConnecting = false;
        return;
    }

    _state = TRYING_TO_CONNECT;
    _timeOfLastConnectionAttempt = timeNow;

    if (_client._serverConnectionIndicator)
        _client._serverConnectionIndicator->set(Indicator::IN_PROGRESS);

    // Server details
    std::string serverIP;
    if (cmdLineArgs.contains("server-ip"))
        serverIP = cmdLineArgs.getString("server-ip");
    else {
        serverIP = _client._defaultServerAddress;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_family = AF_INET;

    // Select server port
#ifdef _DEBUG
    auto port = _client.DEBUG_PORT;
#else
    auto port = PRODUCTION_PORT;
#endif
    if (cmdLineArgs.contains("server-port"))
        port = cmdLineArgs.getInt("server-port");
    serverAddr.sin_port = htons(port);

    if (::connect(_socket.getRaw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0) {
        showError("Connection error: "s + toString(WSAGetLastError()));
        _state = CONNECTION_ERROR;
        _client._serverConnectionIndicator->set(Indicator::FAILED);
    } else {
        _client._serverConnectionIndicator->set(Indicator::SUCCEEDED);
        _client.sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
        _state = CONNECTED;
    }

    _client.updateCreateButton(nullptr);
    _client.updateLoginButton(nullptr);

    getNewMessages();

    _aThreadIsConnecting = false;
}

void Connection::showError(const std::string & msg) const {
    Client::instance().showErrorMessage(msg, Color::FAILURE);
}
