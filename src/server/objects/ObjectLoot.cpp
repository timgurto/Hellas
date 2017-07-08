#include "ObjectLoot.h"

#include "Object.h"
#include "../Server.h"

ObjectLoot::ObjectLoot(Object &parent):
    _parent(parent){}

void ObjectLoot::populate(){
    addStrengthItemsToLoot();
    addContainerItemsToLoot();

    // Alert nearby users of loot
    if (empty())
        return;
    const Server &server = Server::instance();
    for (const User *user : server.findUsersInArea(_parent.location()))
        server.sendMessage(user->socket(), SV_LOOTABLE, makeArgs(_parent.serial()));
}

void ObjectLoot::addStrengthItemsToLoot(){
    static const double MATERIAL_LOOT_CHANCE = 0.5;

    const auto &strengthPair = _parent.objType().strengthPair();
    const ServerItem *strengthItem = strengthPair.first;
    size_t strengthQty = strengthPair.second;

    if (strengthItem == nullptr)
        return;

    size_t lootQuantity = 0;
    for (size_t i = 0; i != strengthQty; ++i)
        if (randDouble() < MATERIAL_LOOT_CHANCE)
            ++lootQuantity;

    add(strengthItem, lootQuantity);
}

void ObjectLoot::addContainerItemsToLoot(){
    static const double CONTAINER_LOOT_CHANCE = 0.5;
    if (_parent.hasContainer()){
        auto lootFromContainer = _parent.container().generateLootWithChance(CONTAINER_LOOT_CHANCE);
        add(lootFromContainer);
    }
}
