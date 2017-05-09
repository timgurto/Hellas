#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H

#include "Test.h"
#include "../client/Client.h"

// A wrapper of the client, with full access, used for testing.
class TestClient{

public:
    TestClient();
    static TestClient WithUsername(const std::string &username);
    static TestClient WithData(const std::string &dataPath);
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
    bool knowsConstruction(const std::string &id) const {
            return _client->_knownConstructions.find(id) != _client->_knownConstructions.end(); }
    const ChoiceList &uiBuildList() const { return *_client->_buildList; }
    Target target() { return _client->_target; }
    const std::map<std::string, Avatar*> &otherUsers() const { return _client->_otherUsers; }

    Avatar &getFirstOtherUser();

    Client *operator->(){ return _client; }
    Client &client() { return *_client; }
    void sendMessage(MessageCode code, const std::string &args){ _client->sendMessage(code, args); }
    MessageCode getNextMessage() const;
    bool waitForMessage(MessageCode desiredMsg, ms_t timeout = DEFAULT_TIMEOUT) const;
    void waitForRedraw();

private:
    Client *_client;

    enum StringType{
        USERNAME,
        DATA_PATH
    };

    TestClient(const std::string &string, StringType type);

    void run();
    void stop();
    void loadData(const std::string path){ _client->loadData(path); }

    bool TestClient::messageWasReceivedSince(MessageCode desiredMsg, size_t startingIndex) const;
};

#endif
