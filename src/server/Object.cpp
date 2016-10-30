#include <cassert>

#include "Object.h"
#include "Server.h"
#include "../util.h"

Object::Object(const ObjectType *type, const Point &loc):
_serial(generateSerial()),
_location(loc),
_type(type),
_numUsersGathering(0)
{
    assert(type);
    if (type->yield()) {
        type->yield().instantiate(_contents);
    }

    if (type->containerSlots() != 0)
        _container = ServerItem::vect_t(type->containerSlots());

    if (type->merchantSlots() != 0)
        _merchantSlots = std::vector<MerchantSlot>(type->merchantSlots());
}

Object::Object(size_t serial): // For set/map lookup ONLY
_serial(serial),
_type(nullptr){}

Object::Object(const Point &loc): // For set/map lookup ONLY
_location(loc),
_serial(0),
_type(nullptr){}

bool Object::compareSerial::operator()( const Object *a, const Object *b){
    return a->_serial < b->_serial;
}

bool Object::compareXThenSerial::operator()( const Object *a, const Object *b){
    if (a->_location.x != b->_location.x)
        return a->_location.x < b->_location.x;
    return a->_serial < b->_serial;
}

bool Object::compareYThenSerial::operator()( const Object *a, const Object *b){
    if (a->_location.y != b->_location.y)
        return a->_location.y < b->_location.y;
    return a->_serial < b->_serial;
}

size_t Object::generateSerial() {
    static size_t currentSerial = Server::STARTING_SERIAL;
    return currentSerial++;
}

void Object::contents(const ItemSet &contents){
    _contents = contents;
}

void Object::removeItem(const ServerItem *item, size_t qty){
    assert (_contents[item] >= qty);
    assert (_contents.totalQuantity() >= qty);
    _contents.remove(item, qty);
}

const ServerItem *Object::chooseGatherItem() const{
    assert(!_contents.isEmpty());
    assert(_contents.totalQuantity() > 0);
    
    // Count number of average gathers remaining for each item type.
    size_t totalGathersRemaining = 0;
    std::map<const Item *, size_t> gathersRemaining;
    for (auto item : _contents) {
        size_t qtyRemaining = item.second;
        double gatherSize = type()->yield().gatherMean(toServerItem(item.first));
        size_t remaining = static_cast<size_t>(ceil(qtyRemaining / gatherSize));
        gathersRemaining[item.first] = remaining;
        totalGathersRemaining += remaining;
    }
    // Choose random item, weighted by remaining gathers.
    size_t i = rand() % totalGathersRemaining;
    for (auto item : gathersRemaining){
        if (i <= item.second)
            return toServerItem(item.first);
        else
            i -= item.second;
    }
    assert(false);
    return 0;
}

size_t Object::chooseGatherQuantity(const ServerItem *item) const{
    size_t randomQty = _type->yield().generateGatherQuantity(item);
    size_t qty = min<size_t>(randomQty, _contents[item]);
    return qty;
}

bool Object::userHasAccess(const std::string &username) const{
    return
        _owner.empty() ||
        _owner == username;
}

void Object::removeItems(const ItemSet &items) {
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
    for (const std::string &username : _watchers) {
        const User &user = Server::instance().getUserByName(username);
        for (size_t slotNum : invSlotsChanged) {
            Server::instance().sendInventoryMessage(user, slotNum, *this);
        }
    }
}



void Object::giveItem(const ServerItem *item, size_t qty){
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

    for (const std::string &username : _watchers){
        const User &user = Server::instance().getUserByName(username);
        for (size_t slot : changedSlots)
            Server::instance().sendInventoryMessage(user, slot, *this);
    }
}

void Object::addWatcher(const std::string &username){
    _watchers.insert(username);
    Server::debug() << username << " is now watching an object." << Log::endl;
}

void Object::removeWatcher(const std::string &username){
    _watchers.erase(username);
    Server::debug() << username << " is no longer watching an object." << Log::endl;
}

bool Object::isContainerEmpty() const{
    for (auto pair : _container)
        if (pair.first != nullptr)
            return false;
    return true;
}

void Object::markForRemoval(){
    Server::_instance->_objectsToRemove.push_back(this);
}

void Object::incrementGatheringUsers(const User *userToSkip){
    const Server &server = *Server::_instance;
    ++_numUsersGathering;
    server._debug << Color::CYAN << _numUsersGathering << Log::endl;
    if (_numUsersGathering == 1){
        for (const User *user : server.findUsersInArea(location()))
            if (user != userToSkip)
                server.sendMessage(user->socket(), SV_GATHERING_OBJECT, makeArgs(_serial));
    }
}

void Object::decrementGatheringUsers(const User *userToSkip){
    const Server &server = *Server::_instance;
    --_numUsersGathering;
    server._debug << Color::CYAN << _numUsersGathering << Log::endl;
    if (_numUsersGathering == 0){
        for (const User *user : server.findUsersInArea(location()))
            if (user != userToSkip)
                server.sendMessage(user->socket(), SV_NOT_GATHERING_OBJECT, makeArgs(_serial));
    }
}
