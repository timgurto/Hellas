#include <cassert>

#include "Container.h"
#include "Object.h"
#include "User.h"
#include "Server.h"
#include "../util.h"

ContainerType *ContainerType::WithSlots(size_t slots) {
    return new ContainerType(slots);
}

ContainerType::ContainerType(size_t slots):
    _slots(slots)
{}

Container *ContainerType::instantiate(Object &parent) const{
    Container *p = new Container(parent);
    p->_container = ServerItem::vect_t(_slots, std::make_pair(nullptr, 0));
    return p;
}



Container::Container(Object &parent):
    _parent(parent)
{}

bool Container::isEmpty() const{
    for (auto pair : _container)
        if (pair.first != nullptr)
            return false;
    return true;
}

void Container::removeItems(const ItemSet &items) {
    std::set<size_t> invSlotsChanged;
    ItemSet remaining = items;
    for (size_t i = 0; i != _container.size(); ++i){
        std::pair<const ServerItem *, size_t> &invSlot = _container[i];
        if (remaining.contains(invSlot.first)) {
            size_t itemsToRemove = min(invSlot.second, remaining[invSlot.first]);
            remaining.remove(invSlot.first, itemsToRemove);
            _container[i].second -= itemsToRemove;
            if (_container[i].second == 0)
                _container[i].first = 0;
            invSlotsChanged.insert(i);
            if (remaining.isEmpty())
                break;
        }
    }
    for (const std::string &username : _parent._watchers) {
        const User &user = Server::instance().getUserByName(username);
        for (size_t slotNum : invSlotsChanged) {
            Server::instance().sendInventoryMessage(user, slotNum, _parent);
        }
    }
}

void Container::addItems(const ServerItem *item, size_t qty){
    std::set<size_t> changedSlots;
    // First pass: partial stacks
    for (size_t i = 0; i != _container.size(); ++i) {
        if (_container[i].first != item)
            continue;
        size_t spaceAvailable = item->stackSize() - _container[i].second;
        if (spaceAvailable > 0) {
            size_t qtyInThisSlot = min(spaceAvailable, qty);
            _container[i].second += qtyInThisSlot;
            changedSlots.insert(i);
            qty -= qtyInThisSlot;
        }
        if (qty == 0)
            break;;
    }

    // Second pass: empty slots
    if (qty != 0)
        for (size_t i = 0; i != _container.size(); ++i) {
            if (_container[i].first)
                continue;
            size_t qtyInThisSlot = min(item->stackSize(), qty);
            _container[i].first = item;
            _container[i].second = qtyInThisSlot;
            changedSlots.insert(i);
            qty -= qtyInThisSlot;
            if (qty == 0)
                break;;
        }
    assert(qty == 0);

    for (const std::string &username : _parent._watchers){
        const User &user = Server::instance().getUserByName(username);
        for (size_t slot : changedSlots)
            Server::instance().sendInventoryMessage(user, slot, _parent);
    }
}

bool Container::isAbleToDeconstruct(const User &user) const{
    if (! isEmpty()){
        Server::instance().sendMessage(user.socket(), SV_NOT_EMPTY);
        return false;
    }
    return true;
}
