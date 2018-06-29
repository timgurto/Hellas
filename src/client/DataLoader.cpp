#include "Client.h"
#include "DataLoader.h"

DataLoader::DataLoader(Client &client, const Directory &dataDirectory):
    _client(client),
    _dataDirectory(dataDirectory){}

void DataLoader::load(bool keepOldData) {
    _client.loadData(_dataDirectory, keepOldData);
}
