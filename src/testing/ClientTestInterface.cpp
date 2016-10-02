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
