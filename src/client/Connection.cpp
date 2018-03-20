#include "Client.h"
#include "Connection.h"
#include "../Args.h"
#include "../curlUtil.h"

extern Args cmdLineArgs;

std::string Connection::defaultServerIP{};

Connection::Connection(Client & client):
_client(client)
{}

void Connection::initialize(const std::string &serverIP) {
    defaultServerIP = readFromURL(serverIP);
}

void Connection::getNewMessages() {
    auto readFDs = fd_set{};
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    auto selectTimeout = timeval{ 0, 10000 };
    auto activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        showError("Error polling sockets: "s + toString(WSAGetLastError()));
        return;
    }
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        const auto BUFFER_SIZE = 1023;
        char buffer[BUFFER_SIZE + 1];
        auto charsRead = recv(_socket.getRaw(), buffer, BUFFER_SIZE, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0) {
            buffer[charsRead] = '\0';
            _client._messages.push({ buffer });
        }
    }
}

void Connection::connect() {
    if (_state == CONNECTED) {
        _aThreadIsConnecting = false;
        return;
    }

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
    auto serverAddr = sockaddr_in{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(getServerIP().c_str());
    serverAddr.sin_port = htons(getServerPort());

    if (::connect(_socket.getRaw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0) {
        auto winsockError = WSAGetLastError();
        showError("Connection error: "s + toString(winsockError));
        const auto ALREADY_CONNECTED = 10056;
        if (winsockError == ALREADY_CONNECTED) {
            _state = CONNECTED;
            _client._serverConnectionIndicator->set(Indicator::SUCCEEDED);
        } else {
            _state = CONNECTION_ERROR;
            _client._serverConnectionIndicator->set(Indicator::FAILED);
        }
    } else {
        _client._serverConnectionIndicator->set(Indicator::SUCCEEDED);
        _client.sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
        _state = CONNECTED;
    }

    _client.updateCreateButton(nullptr);
    _client.updateLoginButton(nullptr);

    _aThreadIsConnecting = false;
}

void Connection::showError(const std::string & msg) const {
    Client::instance().showErrorMessage(msg, Color::FAILURE);
    _client.infoWindow(msg);
}

std::string Connection::getServerIP() {
    if (cmdLineArgs.contains("server-ip"))
        return cmdLineArgs.getString("server-ip");
    return defaultServerIP;
}

u_short Connection::getServerPort() {
    // Specified
    if (cmdLineArgs.contains("server-port"))
        return cmdLineArgs.getInt("server-port");

    // Default
#ifdef _DEBUG
    return DEBUG_PORT;
#else
    return PRODUCTION_PORT;
#endif
}
