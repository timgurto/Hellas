#include <cassert>

#include "Server.h"
#include "User.h"

const size_t User::INVENTORY_SIZE = 20;
const size_t User::GEAR_SLOTS = 8;

ObjectType User::OBJECT_TYPE("__clientObjectType__");

Stats User::BASE_STATS;

std::map<User::Class, std::string> User::CLASS_NAMES;
std::map<std::string, User::Class> User::CLASS_CODES;

User::User(const std::string &name, const Point &loc, const Socket &socket):
Combatant(&OBJECT_TYPE, loc, BASE_STATS.health),

_name(name),
_socket(socket),

_action(NO_ACTION),
_actionTime(0),
_actionObject(nullptr),
_actionRecipe(nullptr),
_actionObjectType(nullptr),
_actionSlot(INVENTORY_SIZE),
_actionLocation(0, 0),

_driving(0),

_inventory(INVENTORY_SIZE),
_gear(GEAR_SLOTS),
_lastContact(SDL_GetTicks()),
_stats(BASE_STATS){
    if (!OBJECT_TYPE.collides()){
        OBJECT_TYPE.collisionRect(Rect(-5, -2, 10, 4));
    }
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory[i] = std::make_pair<const ServerItem *, size_t>(0, 0);
}

User::User(const Socket &rhs):
_socket(rhs){}

User::User(const Point &loc):
Combatant(loc){}

void User::init(){
    BASE_STATS.health = 100;
    BASE_STATS.attack = 8;
    BASE_STATS.attackTime = 1000;
    BASE_STATS.speed = 80.0;
    
    CLASS_NAMES[SOLDIER] = "Soldier";
    CLASS_NAMES[MAGUS] =   "Magus";
    CLASS_NAMES[PRIEST] =  "Priest";

    for (auto &pair : CLASS_NAMES)
        CLASS_CODES[pair.second] = pair.first;
}

bool User::compareXThenSerial::operator()( const User *a, const User *b){
    if (a->location().x != b->location().x)
        return a->location().x < b->location().x;
    return a->_socket < b->_socket; // Just need something unique.
}

bool User::compareYThenSerial::operator()( const User *a, const User *b){
    if (a->location().y != b->location().y)
        return a->location().y < b->location().y;
    return a->_socket < b->_socket; // Just need something unique.
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, location().x, location().y);
}

void User::contact(){
    _lastContact = SDL_GetTicks();
}

bool User::alive() const{
    return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const ServerItem *item, size_t quantity){
    // First pass: partial stacks
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != item)
            continue;
        size_t spaceAvailable = item->stackSize() - _inventory[i].second;
        if (spaceAvailable > 0) {
            size_t qtyInThisSlot = min(spaceAvailable, quantity);
            _inventory[i].second += qtyInThisSlot;
            Server::instance().sendInventoryMessage(*this, i, Server::INVENTORY);
            quantity -= qtyInThisSlot;
        }
        if (quantity == 0)
            return 0;
    }

    // Second pass: empty slots
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != nullptr)
            continue;
        size_t qtyInThisSlot = min(item->stackSize(), quantity);
        _inventory[i].first = item;
        _inventory[i].second = qtyInThisSlot;
        Server::instance().sendInventoryMessage(*this, i, Server::INVENTORY);
        quantity -= qtyInThisSlot;
        if (quantity == 0)
            return 0;
    }
    return quantity;
}

void User::cancelAction() {
    if (_action == NO_ACTION)
        return;

    switch(_action){
    case GATHER:
        _actionObject->decrementGatheringUsers();
    }

    Server::instance().sendMessage(_socket, SV_ACTION_INTERRUPTED);
    _action = NO_ACTION;
}

void User::finishAction() {
    if (_action == NO_ACTION)
        return;

    _action = NO_ACTION;
}

void User::beginGathering(Object *obj){
    _action = GATHER;
    _actionObject = obj;
    _actionObject->incrementGatheringUsers();
    assert(obj->type());
    _actionTime = obj->type()->gatherTime();
}

void User::beginCrafting(const Recipe &recipe){
    _action = CRAFT;
    _actionRecipe = &recipe;
    _actionTime = recipe.time();
}

void User::beginConstructing(const ObjectType &obj, const Point &location, size_t slot){
    _action = CONSTRUCT;
    _actionObjectType = &obj;
    _actionTime = obj.constructionTime();
    _actionSlot = slot;
    _actionLocation = location;
}

void User::beginDeconstructing(Object &obj){
    _action = DECONSTRUCT;
    _actionObject = &obj;
    _actionTime = obj.type()->deconstructionTime();
}

void User::targetNPC(NPC *npc){
    target(npc);
    if (npc == nullptr){
        cancelAction();
        return;
    }
    _action = ATTACK;
}

bool User::hasItems(const ItemSet &items) const{
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        const std::pair<const ServerItem *, size_t> &invSlot = _inventory[i];
        remaining.remove(invSlot.first, invSlot.second);
        if (remaining.isEmpty())
            return true;
    }
    return false;
}

bool User::hasTool(const std::string &tagName) const{
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const ServerItem *item = _inventory[i].first;
        if (item && item->isTag(tagName))
            return true;
    }

    // Check nearby objects
    auto superChunk = Server::_instance->getCollisionSuperChunk(location());
    for (CollisionChunk *chunk : superChunk)
        for (const auto &ret : chunk->objects()) {
            const Object *pObj = ret.second;
            if (pObj->type()->isTag(tagName) &&
                distance(pObj->collisionRect(), collisionRect()) < Server::ACTION_DISTANCE)
                return true;
        }

    return false;
}

bool User::hasTools(const std::set<std::string> &classes) const{
    for (const std::string &tagName : classes)
        if (!hasTool(tagName))
            return false;
    return true;
}

void User::removeItems(const ItemSet &items) {
    std::set<size_t> invSlotsChanged;
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::pair<const ServerItem *, size_t> &invSlot = _inventory[i];
        if (remaining.contains(invSlot.first)) {
            size_t itemsToRemove = min(invSlot.second, remaining[invSlot.first]);
            remaining.remove(invSlot.first, itemsToRemove);
            _inventory[i].second -= itemsToRemove;
            if (_inventory[i].second == 0)
                _inventory[i].first = nullptr;
            invSlotsChanged.insert(i);
            if (remaining.isEmpty())
                break;
        }
    }
    for (size_t slotNum : invSlotsChanged) {
        const std::pair<const ServerItem *, size_t> &slot = _inventory[slotNum];
        std::string id = slot.first ? slot.first->id() : "none";
        Server::instance().sendInventoryMessage(*this, slotNum, Server::INVENTORY);
    }
}

void User::update(ms_t timeElapsed){
    if (_action == NO_ACTION)
        return;

    if (_actionTime > timeElapsed)
        _actionTime -= timeElapsed;
    else
        _actionTime = 0;

    // Attack actions:
    if (_action == ATTACK){
        Combatant::update(timeElapsed);
        return;
    }


    // Non-attack actions:
    Server &server = *Server::_instance;

    if (_actionTime > 0) // Action hasn't finished yet.
        return;
    
    // Timer has finished; complete action
    switch(_action){
    case GATHER:
        server.gatherObject(_actionObject->serial(), *this);
        break;

    case CRAFT:
    {
        const ServerItem *product = toServerItem(_actionRecipe->product());
        if (!vectHasSpace(_inventory, product)) {
            server.sendMessage(_socket, SV_INVENTORY_FULL);
            cancelAction();
            return;
        }
        // Give user his newly crafted item
        giveItem(product, 1);
        // Remove materials from user's inventory
        removeItems(_actionRecipe->materials());
        break;
    }

    case CONSTRUCT:
    {
        // Create object
        server.addObject(_actionObjectType, _actionLocation, this);
        // Remove item from user's inventory
        std::pair<const ServerItem *, size_t> &slot = _inventory[_actionSlot];
        assert(slot.first->constructsObject() == _actionObjectType);
        --slot.second;
        if (slot.second == 0)
            slot.first = nullptr;
        server.sendInventoryMessage(*this, _actionSlot, Server::INVENTORY);
        break;
    }

    case DECONSTRUCT:
    {
        //Check for inventory space
        const ServerItem *item = _actionObject->type()->deconstructsItem();
        if (!vectHasSpace(_inventory, item)){
            server.sendMessage(_socket, SV_INVENTORY_FULL);
            cancelAction();
            return;
        }
        // Give user his item
        giveItem(item);
        // Remove object
        server.removeObject(*_actionObject);
        break;
    }

    default:
        assert(false);
    }
    
    if (_action != ATTACK){ // ATTACK is a repeating action.
        server.sendMessage(_socket, SV_ACTION_FINISHED);
        finishAction();
    }
}

const Rect User::collisionRect() const{
    return OBJECT_TYPE.collisionRect() + location();
}

void User::onHealthChange(){
    const Server &server = *Server::_instance;
    server.sendMessage(socket(), SV_HEALTH, makeArgs(health()));
}

void User::onDeath(){
    // Each game would need to implement this depending on preferred mechanics.
    health(maxHealth());
    onHealthChange();
}

void User::updateStats(){
    health_t oldMaxHealth = maxHealth();

    _stats = BASE_STATS;
    for (size_t i = 0; i != GEAR_SLOTS; ++i){
        const ServerItem *item = _gear[i].first;
        if (item != nullptr)
            _stats &= item->stats();
    }

    // Special case: health must change to reflect new max health
    int healthDecrease = oldMaxHealth - maxHealth();
    if (healthDecrease > 0 && healthDecrease > static_cast<int>(health()))
        health(1); // Implicit rule: changing gear can never kill you, only reduce you to 1 health.
    else
        reduceHealth(healthDecrease);
    if (healthDecrease != 0)
        onHealthChange();

    const Server &server = *Server::_instance;
    server.sendMessage(socket(), SV_STATS, makeArgs(_stats.health, _stats.attack,
                                                    _stats.attackTime, _stats.speed));
}
