#include "TestClient.h"

#include <cassert>
#include <thread>

#include "../threadNaming.h"

TestClient::TestClient() : _client(new Client) {
  CDataLoader::FromPath(*_client, "testing/data/minimal").load();
  _client->setRandomUsername();
  _client->_shouldAutoLogIn = true;
  run();
}

TestClient::TestClient(const StringMap &strings) : _client(new Client) {
  CDataLoader::FromPath(*_client, "testing/data/minimal").load();

  if (strings.count(USERNAME) == 1)
    _client->_username = strings.at(USERNAME);
  else
    _client->setRandomUsername();

  if (strings.count(DATA_PATH) == 1)
    CDataLoader::FromPath(*_client, "testing/data/" + strings.at(DATA_PATH))
        .load(true);
  if (strings.count(DATA_STRING) == 1)
    CDataLoader::FromString(*_client, strings.at(DATA_STRING)).load(true);

  if (strings.count(CLASS) == 1) _client->_autoClassID = strings.at(CLASS);

  _client->_shouldAutoLogIn = true;
  run();
}

TestClient TestClient::WithUsername(const std::string &username) {
  auto strings = StringMap{};
  strings[USERNAME] = username;
  return TestClient(strings);
}

TestClient TestClient::WithData(const std::string &dataPath) {
  auto strings = StringMap{};
  strings[DATA_PATH] = dataPath;
  return TestClient(strings);
}

TestClient TestClient::WithDataString(const std::string &data) {
  auto strings = StringMap{};
  strings[DATA_STRING] = data;
  return TestClient(strings);
}

TestClient TestClient::WithUsernameAndData(const std::string &username,
                                           const std::string &dataPath) {
  auto strings = StringMap{};
  strings[USERNAME] = username;
  strings[DATA_PATH] = dataPath;
  return TestClient(strings);
}

TestClient TestClient::WithUsernameAndDataString(const std::string &username,
                                                 const std::string &data) {
  auto strings = StringMap{};
  strings[USERNAME] = username;
  strings[DATA_STRING] = data;
  return TestClient(strings);
}

TestClient TestClient::WithClassAndDataString(const std::string &classID,
                                              const std::string &data) {
  auto strings = StringMap{};
  strings[CLASS] = classID;
  strings[DATA_STRING] = data;
  return TestClient(strings);
}

TestClient::~TestClient() {
  if (_client == nullptr) return;
  stop();
  delete _client;
}

TestClient::TestClient(TestClient &rhs) : _client(rhs._client) {
  rhs._client = nullptr;
}

TestClient &TestClient::operator=(TestClient &rhs) {
  if (this == &rhs) return *this;
  delete _client;
  _client = rhs._client;
  rhs._client = nullptr;
  return *this;
}

void TestClient::loadDataFromString(const std::string &data) {
  CDataLoader::FromString(*_client, data).load(true);
}

void TestClient::run() {
  std::thread([this]() {
    setThreadName("Client ("s + _client->username() + ")");
    _client->run();
  }).detach();
}

void TestClient::stop() {
  _client->_loop = false;
  _client->_freeze = false;
  WAIT_UNTIL(!_client->_running);
  WAIT_UNTIL(connectionState() != Connection::TRYING_TO_CONNECT);
}

void TestClient::freeze() { _client->_freeze = true; }

const Sprite &TestClient::getFirstNonAvatarSprite() const {
  for (const auto *sprite : _client->_entities) {
    const auto *asAvatar = dynamic_cast<const Avatar *>(sprite);
    if (!asAvatar) return *sprite;
  }
  return *(Sprite *)(nullptr);
}

void TestClient::waitForRedraw() {
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

    if (newSize > oldSize) break;
  }

  if (newSize == oldSize) return NO_CODE;

  _client->_messagesReceivedMutex.lock();
  auto lastMessageCode = _client->_messagesReceived.back();
  _client->_messagesReceivedMutex.unlock();

  return lastMessageCode;
}

bool TestClient::waitForMessage(MessageCode desiredMsg, ms_t timeout) const {
  _client->_messagesReceivedMutex.lock();
  size_t currentSize = _client->_messagesReceived.size();
  _client->_messagesReceivedMutex.unlock();

  for (ms_t startTime = SDL_GetTicks(); SDL_GetTicks() < startTime + timeout;) {
    if (messageWasReceivedSince(desiredMsg, currentSize)) return true;
  }
  return false;
}

bool TestClient::messageWasReceivedSince(MessageCode desiredMsg,
                                         size_t startingIndex) const {
  std::lock_guard<std::mutex> guard(_client->_messagesReceivedMutex);
  const size_t NUM_MESSAGES = _client->_messagesReceived.size();
  if (NUM_MESSAGES > _client->_messagesReceived.size())
    FAIL("Message-count inconsistency");
  if (startingIndex >= NUM_MESSAGES) return false;
  for (size_t i = startingIndex; i != NUM_MESSAGES; ++i)
    if (_client->_messagesReceived[i] == desiredMsg) return true;
  return false;
}

void TestClient::showCraftingWindow() {
  _client->_craftingWindow->show();
  WAIT_UNTIL(!_client->_craftingWindow->changed());
}

bool TestClient::knowsSpell(const std::string &id) const {
  for (const auto *spell : _client->_knownSpells) {
    if (spell->id() == id) return true;
  }
  return false;
}

Avatar &TestClient::getFirstOtherUser() {
  auto otherUsers = _client->_otherUsers;
  REQUIRE(!otherUsers.empty());
  return const_cast<Avatar &>(*otherUsers.begin()->second);
}

ClientNPC &TestClient::getFirstNPC() {
  auto objects = _client->_objects;
  REQUIRE(!objects.empty());
  auto it = objects.begin();
  ClientObject *obj = it->second;
  return *dynamic_cast<ClientNPC *>(obj);
}

ClientObject &TestClient::getFirstObject() {
  auto objects = _client->_objects;
  REQUIRE(!objects.empty());
  auto it = objects.begin();
  return *it->second;
}

const ClientObjectType &TestClient::getFirstObjectType() {
  auto types = _client->gameData.objectTypes;
  REQUIRE(!types.empty());
  auto it = types.begin();
  return **it;
}

const CQuest &TestClient::getFirstQuest() {
  auto &quests = _client->gameData.quests;
  REQUIRE(!quests.empty());
  auto it = quests.begin();
  return it->second;
}

CDroppedItem &TestClient::getFirstDroppedItem() {
  CDroppedItem *asDroppedItem = {nullptr};

  for (auto *sprite : _client->_entities) {
    asDroppedItem = dynamic_cast<CDroppedItem *>(sprite);
    if (asDroppedItem) return *asDroppedItem;
  }

  FAIL("There are no dropped items.");
  return *asDroppedItem;  // To circumvent warning
}

void TestClient::simulateClick(const ScreenPoint &position) {
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

void TestClient::simulateKeypress(SDL_Scancode key) {
  _client->_keyboardState.startFakeKeypress(key);
}

bool TestClient::isKeyPressed(SDL_Scancode key) const {
  return _client->_keyboardState.isPressed(key);
}

void TestClient::sendLocationUpdatesInstantly() {
  _client->TIME_BETWEEN_LOCATION_UPDATES = 0;
}
