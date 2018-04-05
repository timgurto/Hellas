#include <cassert>
#include <thread>

#include "TestClient.h"

TestClient::TestClient():
_client(new Client){
    _client->loadData("testing/data/minimal");
    _client->setRandomUsername();
    _client->_shouldAutoLogIn = true;
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
    _client->_shouldAutoLogIn = true;
    run();
}

TestClient::TestClient(const std::string &username, const std::string &dataPath):
_client(new Client){
    _client->loadData("testing/data/minimal");
    _client->loadData("testing/data/" + dataPath);
    _client->_username = username;
    _client->_shouldAutoLogIn = true;
    run();
}

TestClient TestClient::WithUsername(const std::string &username){
    stopClientIfRunning();
    return TestClient(username, USERNAME);
}

TestClient TestClient::WithData(const std::string &dataPath){
    stopClientIfRunning();
    return TestClient(dataPath, DATA_PATH);
}

TestClient TestClient::WithUsernameAndData(const std::string &username, const std::string &dataPath){
    stopClientIfRunning();
    return TestClient(username, dataPath);
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
}

void TestClient::stop(){
    _client->_loop = false;
    _client->_freeze = false;
    WAIT_UNTIL(!_client->_running);
    WAIT_UNTIL(connectionState() != Connection::TRYING_TO_CONNECT);
}

void TestClient::freeze(){
    _client->_freeze = true;
}

void TestClient::stopClientIfRunning() {
    auto client = Client::_instance;
    if (client && client->_running) {
        client->_loop = false;
        client->_freeze = false;
        WAIT_UNTIL(!client->_running);
        WAIT_UNTIL(client->_connection.state() != Connection::TRYING_TO_CONNECT);
    }
}

void TestClient::waitForRedraw(){
    _client->_drawingFinished = false;
    WAIT_UNTIL(_client->_drawingFinished);
}

MessageCode TestClient::getNextMessage() const {
    _client->_messagesReceivedMutex.lock();
    auto oldSize = _client->_messagesReceived.size();
    _client->_messagesReceivedMutex.unlock();
    size_t newSize = oldSize;

    REPEAT_FOR_MS(10000) {
        _client->_messagesReceivedMutex.lock();
        newSize = _client->_messagesReceived.size();
        _client->_messagesReceivedMutex.unlock();

        if (newSize > oldSize)
            break;
    }

    if (newSize == oldSize)
        return NO_CODE;

    _client->_messagesReceivedMutex.lock();
    auto lastMessageCode = _client->_messagesReceived.back();
    _client->_messagesReceivedMutex.unlock();
    
    return lastMessageCode;
}

bool TestClient::waitForMessage(MessageCode desiredMsg, ms_t timeout) const {
    _client->_messagesReceivedMutex.lock();
    size_t currentSize = _client->_messagesReceived.size();
    _client->_messagesReceivedMutex.unlock();

    for (ms_t startTime = SDL_GetTicks(); SDL_GetTicks() < startTime + timeout; ){
        if (messageWasReceivedSince(desiredMsg, currentSize))
            return true;
    }
    return false;
}

bool TestClient::messageWasReceivedSince(MessageCode desiredMsg, size_t startingIndex) const{
    std::lock_guard<std::mutex> guard(_client->_messagesReceivedMutex);
    const size_t NUM_MESSAGES = _client->_messagesReceived.size();
    if (NUM_MESSAGES > _client->_messagesReceived.size())
        FAIL("Message-count inconsistency");
    if (startingIndex >= NUM_MESSAGES)
        return false;
    for (size_t i = startingIndex; i != NUM_MESSAGES; ++i)
        if (_client->_messagesReceived[i] == desiredMsg)
            return true;
    return false;
}

void TestClient::showCraftingWindow() {
    _client->_craftingWindow->show();
    WAIT_UNTIL(! _client->_craftingWindow->changed());
}

void TestClient::watchObject(ClientObject &obj){
    _client->watchObject(obj);
}

Avatar &TestClient::getFirstOtherUser(){
    auto otherUsers = _client->_otherUsers;
    assert(! otherUsers.empty());
    return const_cast<Avatar &>(* otherUsers.begin()->second);
}

ClientNPC &TestClient::getFirstNPC() {
    auto objects = _client->_objects;
    assert(! objects.empty());
    auto it =objects.begin();
    ClientObject *obj = it->second;
    return * dynamic_cast<ClientNPC *>(obj);
}

ClientObject &TestClient::getFirstObject() {
    auto objects = _client->_objects;
    assert(! objects.empty());
    auto it =objects.begin();
    return *it->second;
}

const ClientObjectType & TestClient::getFirstObjectType() {
    auto types = _client->_objectTypes;
    assert(!types.empty());
    auto it = types.begin();
    return **it;
}

void TestClient::simulateClick(const ScreenPoint &position){
    const auto oldPosition = _client->_mouse;
    _client->_mouse = position;
    _client->onMouseMove();

    SDL_Event mouseDownEvent, mouseUpEvent;
    mouseDownEvent.type = SDL_MOUSEBUTTONDOWN;
    mouseUpEvent.type = SDL_MOUSEBUTTONUP;
    mouseDownEvent.button.button = mouseUpEvent.button.button = SDL_BUTTON_LEFT;

    SDL_PushEvent(&mouseDownEvent);
    SDL_PushEvent(&mouseUpEvent);

}
