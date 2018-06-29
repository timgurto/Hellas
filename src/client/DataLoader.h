#pragma once

#include <string>

class Client;

class DataLoader {
public:
    using Directory = std::string;
    DataLoader(Client &client, const Directory &dataDirectory);

    void load(bool keepOldData = false);

private:
    Client &_client;
    Directory _dataDirectory;
};
