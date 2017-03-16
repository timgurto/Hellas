#include <thread>

#include "ClientTestInterface.h"
#include "Test.h"

ClientTestInterface::ClientTestInterface(){
    _client.loadData("testing/data/minimal");
    _client.setRandomUsername();
}

void ClientTestInterface::run(){
    Client &client = _client;
    std::thread([& client](){ client.run(); }).detach();
    WAIT_UNTIL (_client._connectionStatus == Client::IN_LOGIN_SCREEN);
    _client.login(nullptr);
    WAIT_UNTIL (_client._connectionStatus != Client::TRYING_TO_CONNECT);
}

void ClientTestInterface::stop(){
    _client._loop = false;
    WAIT_UNTIL (!_client._running);
}

void ClientTestInterface::waitForRedraw(){
    _client._drawingFinished = false;
    WAIT_UNTIL(_client._drawingFinished);
}

MessageCode ClientTestInterface::getNextMessage() const {
    size_t currentSize = _client._messagesReceived.size();
    WAIT_UNTIL(_client._messagesReceived.size() > currentSize);
    return _client._messagesReceived.back();
}
