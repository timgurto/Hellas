#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H

#include "../client/Client.h"

// A wrapper of the client, with full access, used for testing.
class TestClient{

public:
    TestClient();
    static TestClient Username(const std::string &username);
    static TestClient Data(const std::string &dataPath);
    ~TestClient();

    // Move constructor/assignment
    TestClient(TestClient &rhs);
    TestClient &operator=(TestClient &rhs);

    void run();
    void stop();

    bool connected() const { return _client->_connectionStatus == Client::CONNECTED; }

    std::map<size_t, ClientObject*> &objects() { return _client->_objects; }
    Client::objectTypes_t &objectTypes() { return _client->_objectTypes; }
    const List &recipeList() const { return *_client->_recipeList; }
    void showCraftingWindow();

    Client *operator->(){ return _client; }
    Client &client() { return *_client; }
    void sendMessage(MessageCode code, const std::string &args){ _client->sendMessage(code, args); }
    MessageCode getNextMessage() const;
    bool waitForMessage(MessageCode desiredMsg) const;
    void waitForRedraw();

private:
    Client *_client;

    enum StringType{
        USERNAME,
        DATA_PATH
    };

    TestClient(const std::string &string, StringType type);

    void loadData(const std::string path){ _client->loadData(path); }

    bool TestClient::messageWasReceivedSince(MessageCode desiredMsg, size_t startingIndex) const;
};

#endif
