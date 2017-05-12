#include <cassert>

#include "Object.h"
#include "Server.h"
#include "../util.h"

Object::Object(const ObjectType *type, const Point &loc):
_serial(generateSerial()),
_location(loc),
_spawner(nullptr),
_numUsersGathering(0),
_lastLocUpdate(SDL_GetTicks()),
_remainingMaterials(type->materials()),
_transformTimer(0),
_container(nullptr),
_deconstruction(nullptr)
{
    if (type != nullptr)
        setType(type);
}

Object::Object(size_t serial): // For set/map lookup ONLY
_serial(serial),
_type(nullptr),
_container(nullptr),
_deconstruction(nullptr){}

Object::Object(const Point &loc): // For set/map lookup ONLY
_location(loc),
_serial(0),
_type(nullptr),
_container(nullptr),
_deconstruction(nullptr){}

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

void Object::addWatcher(const std::string &username){
    _watchers.insert(username);
    Server::debug() << username << " is now watching an object." << Log::endl;
}

void Object::removeWatcher(const std::string &username){
    _watchers.erase(username);
    Server::debug() << username << " is no longer watching an object." << Log::endl;
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

void Object::removeAllGatheringUsers(){
    const Server &server = *Server::_instance;
    _numUsersGathering = 0;
    for (const User *user : server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_NOT_GATHERING_OBJECT, makeArgs(_serial));
}

void Object::onRemove(){
    if (_spawner != nullptr)
        _spawner->scheduleSpawn();
}

void Object::update(ms_t timeElapsed){
    if (isBeingBuilt())
        return;

    // Transform
    if (_transformTimer == 0)
        return;
    if (_type->transformObject() == nullptr)
        return;
    if (_type->transformsOnEmpty() && !_contents.isEmpty())
        return;

    if (timeElapsed > _transformTimer)
        _transformTimer = 0;
    else
        _transformTimer -= timeElapsed;

    if (_transformTimer == 0)
        setType(type()->transformObject());
}

void Object::setType(const ObjectType *type){
    assert(type != nullptr);

    Server *server = Server::_instance;

    _type = type;
    if (type->yield()) {
        type->yield().instantiate(_contents);
    }
    
    server->forceUntarget(*this);
    removeAllGatheringUsers();

    delete _container;
    if (_type->hasContainer()){
        _container = _type->container().instantiate(*this);
    }
    delete _deconstruction;
    if (_type->hasDeconstruction()){
        _deconstruction = _type->deconstruction().instantiate(*this);
    }

    if (type->merchantSlots() != 0)
        _merchantSlots = std::vector<MerchantSlot>(type->merchantSlots());

    if (type->transforms())
        _transformTimer = type->transformTime();

    _remainingMaterials = type->materials();

    // Inform nearby users
    if (server != nullptr)
    for (const User *user : server->findUsersInArea(_location)){
        server->sendObjectInfo(*user, *this);
    }
}

bool Object::isAbleToDeconstruct(const User &user) const{
    if (hasContainer())
        return _container->isAbleToDeconstruct(user);
    return true;
}
