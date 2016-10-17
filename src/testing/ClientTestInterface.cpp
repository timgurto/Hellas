// (C) 2016 Tim Gurto

#include <thread>

#include "ClientTestInterface.h"
#include "Test.h"

void ClientTestInterface::run(){
    Client &client = _client;
    std::thread([& client](){ client.run(); }).detach();
    WAIT_UNTIL (_client._connectionStatus != Client::TRYING);
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
