#include <cassert>

#include "Object.h"
#include "../Server.h"
#include "../../util.h"

Object::Object(const ObjectType *type, const Point &loc):
Entity(type, loc, 0),
_numUsersGathering(0),
_transformTimer(0),
_container(nullptr),
_deconstruction(nullptr)
{
    if (type != nullptr){
        setType(type);
        objType().incrementCounter();
    }
}

Object::Object(size_t serial):
    Entity(serial)
{}

Object::Object(const Point &loc):
    Entity(loc)
{}

Object::~Object(){
    objType().decrementCounter();
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
        double gatherSize = objType().yield().gatherMean(toServerItem(item.first));
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
    size_t randomQty = objType().yield().generateGatherQuantity(item);
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

void Object::incrementGatheringUsers(const User *userToSkip){
    const Server &server = *Server::_instance;
    ++_numUsersGathering;
    server._debug << Color::CYAN << _numUsersGathering << Log::endl;
    if (_numUsersGathering == 1){
        for (const User *user : server.findUsersInArea(location()))
            if (user != userToSkip)
                server.sendMessage(user->socket(), SV_GATHERING_OBJECT, makeArgs(serial()));
    }
}

void Object::decrementGatheringUsers(const User *userToSkip){
    const Server &server = *Server::_instance;
    --_numUsersGathering;
    server._debug << Color::CYAN << _numUsersGathering << Log::endl;
    if (_numUsersGathering == 0){
        for (const User *user : server.findUsersInArea(location()))
            if (user != userToSkip)
                server.sendMessage(user->socket(), SV_NOT_GATHERING_OBJECT, makeArgs(serial()));
    }
}

void Object::removeAllGatheringUsers(){
    const Server &server = *Server::_instance;
    _numUsersGathering = 0;
    for (const User *user : server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_NOT_GATHERING_OBJECT, makeArgs(serial()));
}

void Object::update(ms_t timeElapsed){
    if (isBeingBuilt())
        return;

    // Transform
    if (_transformTimer == 0)
        return;
    if (objType().transformObject() == nullptr)
        return;
    if (objType().transformsOnEmpty() && !_contents.isEmpty())
        return;

    if (timeElapsed > _transformTimer)
        _transformTimer = 0;
    else
        _transformTimer -= timeElapsed;

    if (_transformTimer == 0)
        setType(objType().transformObject());
}

void Object::setType(const ObjectType *type){
    assert(type != nullptr);

    Server *server = Server::_instance;

    Entity::type(type);

    if (type->yield()) {
        type->yield().instantiate(_contents);
    }
    
    server->forceAllToUntarget(*this);
    removeAllGatheringUsers();

    delete _container;
    if (objType().hasContainer()){
        _container = objType().container().instantiate(*this);
    }
    delete _deconstruction;
    if (objType().hasDeconstruction()){
        _deconstruction = objType().deconstruction().instantiate(*this);
    }

    if (type->merchantSlots() != 0)
        _merchantSlots = std::vector<MerchantSlot>(type->merchantSlots());

    if (type->transforms())
        _transformTimer = type->transformTime();

    _remainingMaterials = type->materials();

    // Inform nearby users
    if (server != nullptr)
        for (const User *user : server->findUsersInArea(location()))
            sendInfoToClient(*user);
}

bool Object::isAbleToDeconstruct(const User &user) const{
    if (hasContainer())
        return _container->isAbleToDeconstruct(user);
    return true;
}

void Object::sendInfoToClient(const User &targetUser) const {
    const Server &server = Server::instance();
    const Socket &client = targetUser.socket();

    server.sendMessage(client, SV_OBJECT, makeArgs(serial(), location().x, location().y,
                                                   type()->id()));

    // Owner
    if (permissions().hasOwner()){
        const auto &owner = permissions().owner();
        server.sendMessage(client, SV_OWNER, makeArgs(serial(), owner.typeString(), owner.name));
    }

    // Being gathered
    if (numUsersGathering() > 0)
        server.sendMessage(client, SV_GATHERING_OBJECT, makeArgs(serial()));

    // Construction materials
    if (isBeingBuilt()){
        server.sendConstructionMaterialsMessage(targetUser, *this);
    }

    // Transform timer
    if (isTransforming()){
        server.sendMessage(client, SV_TRANSFORM_TIME, makeArgs(serial(), transformTimer()));
    }
}
