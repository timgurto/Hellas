#include "Connection.h"

#include "../Args.h"
#include "../curlUtil.h"
#include "Client.h"

extern Args cmdLineArgs;

Connection::Connection(Client &client) : _client(&client) {}

void Connection::initialize() {
  if (cmdLineArgs.contains("server-ip"))
    serverIP = cmdLineArgs.getString("server-ip");
  else
    serverIP = readFromURL(_client->_config.serverHostDirectory);  // Slow
}

Connection::~Connection() {
  while (_aThreadIsConnecting)
    ;
}

void Connection::getNewMessages() {
  auto readFDs = fd_set{};
  FD_ZERO(&readFDs);
  FD_SET(_socket.getRaw(), &readFDs);
  auto selectTimeout = timeval{0, 10000};
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
      _client->_messages.push({buffer});
    }
  }
}

void Connection::connect() {
  _aThreadIsConnecting = true;

  _state = TRYING_TO_CONNECT;
  auto timeNow = SDL_GetTicks();
  _timeOfLastConnectionAttempt = timeNow;

  if (_client->_serverConnectionIndicator)
    _client->_serverConnectionIndicator->set(Indicator::IN_PROGRESS);

  // Server details
  auto serverAddr = sockaddr_in{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
  serverAddr.sin_port = htons(getServerPort());

  if (::connect(_socket.getRaw(), (sockaddr *)&serverAddr,
                Socket::sockAddrSize) < 0) {
    auto winsockError = WSAGetLastError();
    showError("Connection error: "s + toString(winsockError));
    const auto ALREADY_CONNECTED = 10056;
    if (winsockError == ALREADY_CONNECTED) {
      _state = CONNECTED;
#ifndef TESTING
      _client->_serverConnectionIndicator->set(Indicator::SUCCEEDED);
#endif
    } else {
      _state = CONNECTION_ERROR;
#ifndef TESTING
      _client->_serverConnectionIndicator->set(Indicator::FAILED);
#endif
    }
  } else {
#ifndef TESTING
    _client->_serverConnectionIndicator->set(Indicator::SUCCEEDED);
#endif
    _client->sendMessage({CL_PING, makeArgs(SDL_GetTicks())});
    _state = CONNECTED;
  }

#ifndef TESTING
  _client->updateCreateButton(_client);
  _client->updateLoginButton();
#endif

  _aThreadIsConnecting = false;
}

bool Connection::shouldAttemptReconnection() const {
  if (_state == CONNECTED) return false;

  if (_aThreadIsConnecting) return false;

  auto timeNow = SDL_GetTicks();
  if (timeNow - _timeOfLastConnectionAttempt < TIME_BETWEEN_CONNECTION_ATTEMPTS)
    return false;

  return true;
}

void Connection::showError(const std::string &msg) const {
#ifdef TESTING
  return;
#endif
  _client->_queuedErrorMessagesFromOtherThreads.push_back(msg);
  _client->_queuedToastsFromOtherThreads.push_back(msg);
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
