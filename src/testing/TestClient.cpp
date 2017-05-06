#include <cassert>
#include <thread>

#include "TestClient.h"
#include "Test.h"

TestClient::TestClient():
_client(new Client){
    _client->loadData("testing/data/minimal");
    _client->setRandomUsername();
    run();
}

TestClient::TestClient(const std::string &string, StringType type):
_client(new Client){
    _client->loadData("testing/data/minimal");
    switch(type){
    case USERNAME:
        _client->_username = string;
        break;
    case DATA_PATH:
        _client->setRandomUsername();
        _client->loadData("testing/data/" + string);
        break;
    default:
        assert(false);
    }
    run();
}

TestClient TestClient::Username(const std::string &username){
    return TestClient(username, USERNAME);
}

TestClient TestClient::Data(const std::string &dataPath){
    return TestClient(dataPath, DATA_PATH);
}

TestClient::~TestClient(){
    if (_client == nullptr)
        return;
    stop();
    delete _client;
}

TestClient::TestClient(TestClient &rhs):
_client(rhs._client){
    rhs._client = nullptr;
}

TestClient &TestClient::operator=(TestClient &rhs){
    if (this == &rhs)
        return *this;
    delete _client;
    _client = rhs._client;
    rhs._client = nullptr;
    return *this;
}

void TestClient::run(){
    Client &client = *_client;
    std::thread([& client](){ client.run(); }).detach();
    WAIT_FOREVER_UNTIL (_client->_connectionStatus == Client::IN_LOGIN_SCREEN);
    _client->login(nullptr);
    WAIT_FOREVER_UNTIL (_client->_connectionStatus != Client::TRYING_TO_CONNECT);
}

void TestClient::stop(){
    _client->_loop = false;
    WAIT_FOREVER_UNTIL (!_client->_running);
}

void TestClient::waitForRedraw(){
    _client->_drawingFinished = false;
    WAIT_FOREVER_UNTIL(_client->_drawingFinished);
}

MessageCode TestClient::getNextMessage() const {
    size_t currentSize = _client->_messagesReceived.size();
    WAIT_FOREVER_UNTIL(_client->_messagesReceived.size() > currentSize);
    return _client->_messagesReceived.back();
}

bool TestClient::waitForMessage(MessageCode desiredMsg, ms_t timeout) const {
    size_t currentSize = _client->_messagesReceived.size();
    WAIT_UNTIL_TIMEOUT (messageWasReceivedSince(desiredMsg, currentSize), timeout);
    return true;
}

bool TestClient::messageWasReceivedSince(MessageCode desiredMsg, size_t startingIndex) const{
    const size_t NUM_MESSAGES = _client->_messagesReceived.size();
    if (startingIndex >= NUM_MESSAGES)
        return false;
    for (size_t i = startingIndex; i != NUM_MESSAGES; ++i)
        if (_client->_messagesReceived[i] == desiredMsg)
            return true;
    return false;
}

void TestClient::showCraftingWindow() {
    _client->_craftingWindow->show();
    WAIT_FOREVER_UNTIL(! _client->_craftingWindow->changed());

}

Avatar &TestClient::getFirstOtherUser(){
    assert(! _client->_otherUsers.empty());
    return const_cast<Avatar &>(* _client->_otherUsers.begin()->second);
}
