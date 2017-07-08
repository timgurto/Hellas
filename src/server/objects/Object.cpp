#include <cassert>

#include "Object.h"
#include "../Server.h"
#include "../../util.h"

Object::Object(const ObjectType *type, const Point &loc):
Entity(type, loc, type->strength()),
_numUsersGathering(0),
_transformTimer(0),
_container(nullptr),
_deconstruction(nullptr),
_permissions(*this)
{
    setType(type);
    objType().incrementCounter();
}

Object::Object(size_t serial):
    Entity(serial),
    _permissions(*this)
{}

Object::Object(const Point &loc):
    Entity(loc),
    _permissions(*this)
{}

Object::~Object(){
    if (permissions().hasOwner()){
        Server &server = *Server::_instance;
        server._objectsByOwner.remove(permissions().owner(), serial());
    }

    if (type() != nullptr)
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
    do {
        if (_transformTimer == 0)
            break;
        if (objType().transformObject() == nullptr)
            break;
        if (objType().transformsOnEmpty() && !_contents.isEmpty())
            break;

        if (timeElapsed > _transformTimer)
            _transformTimer = 0;
        else
            _transformTimer -= timeElapsed;

        if (_transformTimer == 0)
            setType(objType().transformObject());
    } while (false);

    Entity::update(timeElapsed);
}

void Object::onHealthChange(){
    const Server &server = *Server::_instance;
    for (const User *user: server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_ENTITY_HEALTH, makeArgs(serial(), health()));
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

void Object::onDeath(){
    Server &server = *Server::_instance;
    server.forceAllToUntarget(*this);

    populateLoot();

    if (hasContainer())
        container().removeAll();

    Entity::onDeath();
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

    // Health
    if (health() < maxHealth())
        server.sendMessage(client, SV_ENTITY_HEALTH, makeArgs(serial(), health()));
}

void Object::populateLoot(){
    addStrengthItemsToLoot();
    addContainerItemsToLoot();

    // Alert nearby users of loot
    if (_loot.empty())
        return;
    const Server &server = Server::instance();
    for (const User *user : server.findUsersInArea(location()))
        server.sendMessage(user->socket(), SV_LOOTABLE, makeArgs(serial()));
}

void Object::addStrengthItemsToLoot(){
    static const double MATERIAL_LOOT_CHANCE = 0.5;

    const auto &strengthPair = objType().strengthPair();
    const ServerItem *strengthItem = objType().strengthPair().first;
    size_t strengthQty = objType().strengthPair().second;

    if (strengthItem == nullptr)
        return;

    size_t lootQuantity = 0;
    for (size_t i = 0; i != strengthQty; ++i)
        if (randDouble() < MATERIAL_LOOT_CHANCE)
            ++lootQuantity;

    _loot.add(strengthItem, lootQuantity);
}

void Object::addContainerItemsToLoot(){
    static const double CONTAINER_LOOT_CHANCE = 0.5;
    if (hasContainer()){
        auto lootFromContainer = container().generateLootWithChance(CONTAINER_LOOT_CHANCE);
        _loot.add(lootFromContainer);
    }
}

void Object::describeSelfToNewWatcher(const User &watcher) const{
    const Server &server = Server::instance();

    // Describe merchant slots, if any
    size_t numMerchantSlots = merchantSlots().size();
    for (size_t i = 0; i != numMerchantSlots; ++i)
        server.sendMerchantSlotMessage(watcher, *this, i);

    // Describe inventory, if user has permission
    if (hasContainer() &&
        permissions().doesUserHaveAccess(watcher.name())){
            size_t slots = objType().container().slots();
            for (size_t i = 0; i != slots; ++i)
                server.sendInventoryMessage(watcher, i, *this);
    }

    _loot.sendContentsToUser(watcher, serial());
}

ServerItem::Slot *Object::getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user){
    const Server &server = Server::instance();
    const Socket &socket = user.socket();

    auto hasLoot = ! _loot.empty();
    if (! (hasLoot || hasContainer())){
        server.sendMessage(socket, SV_NO_INVENTORY);
        return nullptr;
    }

    if (!server.isEntityInRange(socket, user, this))
        return nullptr;

    if (! permissions().doesUserHaveAccess(user.name())){
        server.sendMessage(socket, SV_NO_PERMISSION);
        return nullptr;
    }
       
    if (isBeingBuilt()){
        server.sendMessage(socket, SV_UNDER_CONSTRUCTION);
        return nullptr;
    }

    if (hasLoot){
        ServerItem::Slot &slot = _loot.at(slotNum);
        if (slot.first == nullptr){
            server.sendMessage(socket, SV_EMPTY_SLOT);
            return nullptr;
        }
        return &slot;
    }

    if (slotNum >= objType().container().slots()) {
        server.sendMessage(socket, SV_INVALID_SLOT);
        return nullptr;
    }

    assert(hasContainer());
    ServerItem::Slot &slot = container().at(slotNum);
    if (slot.first == nullptr){
        server.sendMessage(socket, SV_EMPTY_SLOT);
        return nullptr;
    }
    return &slot;
}

void Object::alertWatcherOnInventoryChange(const User &watcher, size_t slot) const{
    const Server &server = Server::instance();

    if (! _loot.empty()){
        _loot.sendSingleSlotToUser(watcher, serial(), slot);

    } else {
        const std::string &username = watcher.name();
        if (! permissions().doesUserHaveAccess(username))
            return;
        if (! hasContainer())
            return;
        server.sendInventoryMessage(watcher, slot, *this);
    }
}

Message Object::outOfRangeMessage() const{
    return Message(SV_OBJECT_OUT_OF_RANGE, makeArgs(serial()));
}

bool Object::shouldAlwaysBeKnownToUser(const User &user) const{
    if (permissions().isOwnedByPlayer(user.name()))
        return true;
    const Server &server = *Server::_instance;
    const auto &city = server.cities().getPlayerCity(user.name());
    if (!city.empty() && permissions().isOwnedByCity(city))
        return true;
    return false;
}
