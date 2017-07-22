#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H

#include "testing.h"
#include "../client/Client.h"

// A wrapper of the client, with full access, used for testing.
class TestClient{

public:
    TestClient();
    static TestClient WithUsername(const std::string &username);
    static TestClient WithData(const std::string &dataPath);
    static TestClient WithUsernameAndData(const std::string &username, const std::string &dataPath);
    ~TestClient();

    // Move constructor/assignment
    TestClient(TestClient &rhs);
    TestClient &operator=(TestClient &rhs);

    bool connected() const { return _client->_connectionStatus == Client::CONNECTED; }
    void freeze();

    std::map<size_t, ClientObject*> &objects() { return _client->_objects; }
    Client::objectTypes_t &objectTypes() { return _client->_objectTypes; }
    const List &recipeList() const { return *_client->_recipeList; }
    void showCraftingWindow();
    void watchObject(ClientObject &obj);
    bool knowsConstruction(const std::string &id) const {
            return _client->_knownConstructions.find(id) != _client->_knownConstructions.end(); }
    const ChoiceList &uiBuildList() const { return *_client->_buildList; }
    Target target() { return _client->_target; }
    const std::map<std::string, Avatar*> &otherUsers() const { return _client->_otherUsers; }
    ClientItem::vect_t &inventory() { return _client->_inventory; }
    const std::string &name() const { return _client->username(); }
    const List *chatLog() const { return _client->_chatLog; }
    const Element::children_t &mapPins() const { return _client->_mapPins->children(); }
    const Element::children_t &mapPinOutlines() const { return _client->_mapPinOutlines->children(); }
    const std::vector<std::vector<char> > &map() const { return _client->_map; }
    Window *craftingWindow() const { return _client->_craftingWindow; }
    Window *buildWindow() const { return _client->_buildWindow; }
    Window *gearWindow() const { return _client->_gearWindow; }
    Window *mapWindow() const { return _client->_mapWindow; }
    bool isAtWarWith(const Avatar &user) const { return _client->isAtWarWith(user); }

    Avatar &getFirstOtherUser();
    ClientNPC &getFirstNPC();
    ClientObject &getFirstObject();

    Client *operator->(){ return _client; }
    Client &client() { return *_client; }
    void sendMessage(MessageCode code, const std::string &args){ _client->sendMessage(code, args); }
    MessageCode getNextMessage() const;
    bool waitForMessage(MessageCode desiredMsg, ms_t timeout = DEFAULT_TIMEOUT) const;
    void waitForRedraw();
    void simulateClick(const Point &position);

private:
    Client *_client;

    enum StringType{
        USERNAME,
        DATA_PATH
    };

    TestClient(const std::string &string, StringType type);
    TestClient(const std::string &username, const std::string &dataPath);

    void run();
    void stop();
    void loadData(const std::string path){ _client->loadData(path); }

    bool TestClient::messageWasReceivedSince(MessageCode desiredMsg, size_t startingIndex) const;
};

#endif
