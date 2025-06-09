#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H

#include "../Message.h"
#include "../client/CDataLoader.h"
#include "../client/CDroppedItem.h"
#include "../client/CQuest.h"
#include "../client/Client.h"
#include "testing.h"

class CQuest;
class CCities;

// A wrapper of the client, with full access, used for testing.
class TestClient {
 public:
  TestClient();
  static TestClient WithUsername(const std::string &username);
  static TestClient WithData(const std::string &dataPath);
  static TestClient WithDataString(const std::string &data);
  static TestClient WithUsernameAndData(const std::string &username,
                                        const std::string &dataPath);
  static TestClient WithUsernameAndDataString(const std::string &username,
                                              const std::string &data);
  static TestClient WithClassAndDataString(const std::string &classID,
                                           const std::string &data);
  ~TestClient();

  // Move constructor/assignment
  TestClient(TestClient &rhs);
  TestClient &operator=(TestClient &rhs);

  void loadDataFromString(const std::string &data);

  bool connected() const {
    return _client->_connection.state() == Connection::CONNECTED;
  }
  bool loggedIn() const { return _client->_loggedIn; }
  void freeze();

  std::map<Serial, ClientObject *> &objects() { return _client->_objects; }
  Sprite::set_t &entities() { return _client->_entities; }
  const Sprite &getFirstNonAvatarSprite() const;
  CGameData::ObjectTypes &objectTypes() {
    return _client->gameData.objectTypes;
  }
  const CQuests &quests() const { return _client->gameData.quests; }
  const std::map<std::string, ClientItem> &items() const {
    return _client->gameData.items;
  }
  const List &recipeList() const { return *_client->_recipeList; }
  void showCraftingWindow();
  bool knowsConstruction(const std::string &id) const {
    return _client->_knownConstructions.count(id) == 1;
  }
  bool knowsRecipe(const std::string &id) const {
    return _client->_knownRecipes.count(id) == 1;
  }
  const ChoiceList &uiBuildList() const { return *_client->_buildList; }
  bool knowsSpell(const std::string &id) const;
  Target target() { return _client->_target; }
  const Sprite *entityUnderCursor() const {
    return _client->_currentMouseOverEntity;
  }
  const std::set<std::string> &knownRecipes() const {
    return _client->_knownRecipes;
  }
  const std::map<std::string, Avatar *> &otherUsers() const {
    return _client->_otherUsers;
  }
  const Stats &stats() const { return _client->_stats; }
  ClientItem::vect_t &inventory() { return _client->_inventory; }
  ClientItem::vect_t &gear() { return _client->_character.gear(); }
  const std::string &name() const { return _client->username(); }
  const CurrentTools &currentTools() const { return _client->_currentTools; }
  const List *chatLog() const { return _client->_chatLog; }
  const Element::children_t &mapPins() const {
    return _client->_mapPins->children();
  }
  const Element::children_t &mapPinOutlines() const {
    return _client->_mapPinOutlines->children();
  }
  const Map &map() const { return _client->_map; }
  const std::set<std::string> &allOnlinePlayers() const {
    return _client->_allOnlinePlayers;
  }
  const std::string &allowedTerrain() const { return _client->_allowedTerrain; }
  const CCities &cities() const { return _client->_cities; }
  const MapPoint &respawnPoint() const { return _client->_respawnPoint; }

  Window *craftingWindow() const { return _client->_craftingWindow; }
  Window *buildWindow() const { return _client->_buildWindow; }
  Window *gearWindow() const { return _client->_gearWindow; }
  Window *mapWindow() const { return _client->_mapWindow; }
  bool isAtWarWith(const Avatar &user) const {
    return _client->isAtWarWith(user);
  }
  const std::string &cityName() const {
    return _client->character().cityName();
  }
  const Connection::State connectionState() const {
    return _client->_connection.state();
  }

  Avatar &getFirstOtherUser();
  ClientNPC &getFirstNPC();
  ClientNPC &waitForFirstNPC();
  ClientObject &getFirstObject();
  ClientObject &waitForFirstObject();
  const ClientObjectType &getFirstObjectType();
  const CQuest &getFirstQuest();
  CDroppedItem &getFirstDroppedItem();
  const ClientItem &getFirstItem() const;
  const CRecipe &getFirstRecipe() const;

  const CQuest &findQuest(std::string id) const;

  Client *operator->() { return _client; }
  Client &client() { return *_client; }
  void performCommand(const std::string &command) {
    _client->performCommand(command);
  }
  void sendMessage(MessageCode code, const std::string &args = "") const {
    _client->sendMessage({code, args});
  }
  MessageCode getNextMessage() const;
  bool waitForMessage(MessageCode desiredMsg,
                      ms_t timeout = DEFAULT_TIMEOUT) const;
  void waitForRedraw();
  void simulateMouseMove(const ScreenPoint &position);
  void simulateClick(Uint8 button);
  void simulateKeypress(SDL_Scancode key);
  bool isKeyPressed(SDL_Scancode key) const;
  void sendLocationUpdatesInstantly();
  void startCrafting(const CRecipe *recipe, int quantity);

 private:
  Client *_client;

  enum StringType { USERNAME, CLASS, DATA_PATH, DATA_STRING };
  using StringMap = std::map<StringType, std::string>;
  TestClient(const StringMap &strings);
  TestClient(const std::string &string, StringType type);
  TestClient(const std::string &username, const std::string &string,
             StringType type);

  void run();
  void stop();
  void loadData(const std::string path) {
    CDataLoader::FromPath(*_client, path).load();
  }

  bool TestClient::messageWasReceivedSince(MessageCode desiredMsg,
                                           size_t startingIndex) const;
};

#endif
