#include "Loot.h"
#include "Server.h"
#include "User.h"
#include "../util.h"

bool Loot::empty() const{
    if (_container.size() == 0)
        return true;
    for (const auto &pair : _container){
        const ServerItem *item = pair.first;
        if (item == nullptr)
            continue;
        size_t quantity = pair.second;
        if (quantity > 0)
            return false;
    }
    return true;
}

void Loot::add(const ServerItem *item, size_t qty){
    size_t stackSize = item->stackSize();
    size_t remainingQuantity = qty;
    while (remainingQuantity > 0){
        size_t quantityInThisSlot = min(stackSize, remainingQuantity);
        std::pair<const ServerItem *, size_t> entry;
        entry.first = item;
        entry.second = quantityInThisSlot;
        remainingQuantity -= quantityInThisSlot;
        _container.push_back(entry);
    }
}

void Loot::sendContentsToUser(const User &recipient, size_t serial) const{
    const Server &server = Server::instance();
    server.sendMessage(recipient.socket(), SV_LOOT_COUNT, makeArgs(serial, _container.size()));
    for (size_t i = 0; i != _container.size(); ++i)
        server.sendInventoryMessageInner(recipient, serial, i, _container);
}
