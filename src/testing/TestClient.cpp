#include <thread>

#include "TestClient.h"
#include "Test.h"

TestClient::TestClient(const std::string &username){
    _client.loadData("testing/data/minimal");
    if (username.empty())
        _client.setRandomUsername();
    else
        _client._username = username;
}

void TestClient::run(){
    Client &client = _client;
    std::thread([& client](){ client.run(); }).detach();
    WAIT_UNTIL (_client._connectionStatus == Client::IN_LOGIN_SCREEN);
    _client.login(nullptr);
    WAIT_UNTIL (_client._connectionStatus != Client::TRYING_TO_CONNECT);
}

void TestClient::stop(){
    _client._loop = false;
    WAIT_UNTIL (!_client._running);
}

void TestClient::waitForRedraw(){
    _client._drawingFinished = false;
    WAIT_UNTIL(_client._drawingFinished);
}

MessageCode TestClient::getNextMessage() const {
    size_t currentSize = _client._messagesReceived.size();
    WAIT_UNTIL(_client._messagesReceived.size() > currentSize);
    return _client._messagesReceived.back();
}

void TestClient::showCraftingWindow() {
    _client._craftingWindow->show();
    WAIT_UNTIL(! _client._craftingWindow->changed());

}
