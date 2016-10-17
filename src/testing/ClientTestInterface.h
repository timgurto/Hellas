// (C) 2016 Tim Gurto

#ifndef CLIENT_TEST_INTERFACE_H
#define CLIENT_TEST_INTERFACE_H

#include "../client/Client.h"

// A wrapper of the client, with full access, used for testing.
class ClientTestInterface{
    Client _client;

public:
    void run();
    void stop();

    ~ClientTestInterface(){ stop(); }

    bool connected() const { return _client._connectionStatus == Client::CONNECTED; }

    std::map<size_t, ClientObject*> &objects() { return _client._objects; }
    Client::objectTypes_t &objectTypes() { return _client._objectTypes; }

    Client *operator->(){ return &_client; }
    Client &client() { return _client; }
    void loadData(const std::string path){ _client.loadData(path); }
    void sendMessage(MessageCode code, const std::string &args){ _client.sendMessage(code, args); }
    MessageCode getNextMessage() const;

    void waitForRedraw();
};

#endif
