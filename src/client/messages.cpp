#include <cassert>

#include "Client.h"
#include "ClientCombatant.h"
#include "ClientObject.h"
#include "ClientNPC.h"
#include "ClientVehicle.h"
#include "ui/Container.h"

static void readString(std::istream &iss, std::string &str, char delim = MSG_DELIM){
    if (iss.peek() == delim) {
        str = "";
    } else {
        static const size_t BUFFER_SIZE = 1023;
        static char buffer[BUFFER_SIZE+1];
        iss.get(buffer, BUFFER_SIZE, delim);
        str = buffer;
    }
}

std::istream &operator>>(std::istream &lhs, std::string &rhs){
    readString(lhs, rhs, MSG_DELIM);
    return lhs;
}

void Client::handleMessage(const std::string &msg){
    _partialMessage.append(msg);
    std::istringstream iss(_partialMessage);
    _partialMessage = "";
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];

    // Read while there are new messages
    while (!iss.eof()) {
        // Discard malformed data
        if (iss.peek() != MSG_START) {
            iss.get(buffer, BUFFER_SIZE, MSG_START);
            _debug << "Read " << iss.gcount() << " characters." << Log::endl;
            _debug << Color::FAILURE << "Malformed message; discarded \""
                   << buffer << "\"" << Log::endl;
            if (iss.eof()) {
                break;
            }
        }

        // Get next message
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        if (iss.eof()){
            _partialMessage = buffer;
            break;
        } else {
            std::streamsize charsRead = iss.gcount();
            buffer[charsRead] = MSG_END;
            buffer[charsRead+1] = '\0';
            iss.ignore(); // Throw away ']'
        }
        std::istringstream singleMsg(buffer);
        //_debug(buffer, Color::CYAN);
        singleMsg >> del >> msgCode >> del;
        _messagesReceived.push_back(MessageCode(msgCode));
        Color errorMessageColor = Color::FAILURE;

        switch(msgCode) {

        case SV_WELCOME:
        {
            if (del != MSG_END)
                break;
            _connectionStatus = LOGGED_IN;
            _loggedIn = true;
            _timeSinceConnectAttempt = 0;
            _lastPingSent = _lastPingReply = _time;
            _charPin->setTooltip(_username);
            _debug("Successfully logged in to server", Color::SUCCESS);
            break;
        }

        case SV_PING_REPLY:
        {
            ms_t timeSent;
            singleMsg >> timeSent >> del;
            if (del != MSG_END)
                break;
            _lastPingReply = _time;
            _latency = (_time - timeSent) / 2;
            break;
        }

        case SV_USER_DISCONNECTED:
        case SV_USER_OUT_OF_RANGE:
        {
            std::string name;
            readString(singleMsg, name, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            const std::map<std::string, Avatar*>::iterator it = _otherUsers.find(name);
            if (it != _otherUsers.end()) {

                // Remove as driver from vehicle
                if (it->second->isDriving())
                    for (auto &objPair : _objects)
                        if (objPair.second->classTag() == 'v'){
                            ClientVehicle &v = dynamic_cast<ClientVehicle &>(*objPair.second);
                            if (v.driver() == it->second){
                                v.driver(nullptr);
                                break;
                            }
                        }

                removeEntity(it->second);
                _otherUsers.erase(it);
            }
            if (msgCode == SV_USER_DISCONNECTED)
                _debug << name << " disconnected." << Log::endl;
            break;
        }

        case SV_DUPLICATE_USERNAME:
            if (del != MSG_END)
                break;
            _invalidUsername = true;
            _debug << Color::FAILURE << "The user " << _username
                   << " is already connected to the server." << Log::endl;
            break;

        case SV_INVALID_USERNAME:
            if (del != MSG_END)
                break;
            _invalidUsername = true;
            _debug << Color::FAILURE << "The username " << _username << " is invalid." << Log::endl;
            break;

        case SV_SERVER_FULL:
            _socket = Socket();
            _loggedIn = false;
            _debug(_errorMessages[msgCode], Color::FAILURE);
            break;

        case SV_TOO_FAR:
        case SV_DOESNT_EXIST:
        case SV_NEED_MATERIALS:
        case SV_NEED_TOOLS:
        case SV_ACTION_INTERRUPTED:
        case SV_BLOCKED:
        case SV_INVENTORY_FULL:
        case SV_NO_PERMISSION:
        case SV_NO_WARE:
        case SV_NO_PRICE:
        case SV_MERCHANT_INVENTORY_FULL:
        case SV_NOT_EMPTY:
        case SV_VEHICLE_OCCUPIED:
        case SV_NO_VEHICLE:
        case SV_WRONG_MATERIAL:
            errorMessageColor = Color::WARNING; // Yellow above, red below
        case SV_INVALID_USER:
        case SV_INVALID_ITEM:
        case SV_CANNOT_CRAFT:
        case SV_INVALID_SLOT:
        case SV_EMPTY_SLOT:
        case SV_CANNOT_CONSTRUCT:
        case SV_NOT_MERCHANT:
        case SV_INVALID_MERCHANT_SLOT:
        case SV_NPC_DEAD:
        case SV_NPC_SWAP:
        case SV_TAKE_SELF:
        case SV_NOT_GEAR:
        case SV_NOT_VEHICLE:
        case SV_UNKNOWN_RECIPE:
        case SV_UNKNOWN_CONSTRUCTION:
        case SV_UNDER_CONSTRUCTION:
        case SV_AT_PEACE:
            if (del != MSG_END)
                break;
            _debug(_errorMessages[msgCode], errorMessageColor);
            startAction(0);
            break;

        case SV_ITEM_NEEDED:
        {
            std::string reqItemTag;
            readString(singleMsg, reqItemTag, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)

                break;
            std::string msg = "You need a";
            const char first = reqItemTag.front();
            if (first == 'a' || first == 'e' || first == 'i' ||
                first == 'o' || first == 'u')
                msg += 'n';
            _debug(msg + ' ' + reqItemTag + " to do that.", Color::WARNING);
            startAction(0);
            break;
        }

        case SV_ACTION_STARTED:
            ms_t time;
            singleMsg >> time >> del;
            if (del != MSG_END)
                break;
            startAction(time);

            // If constructing, hide footprint now that it has successfully started.
            if (_selectedConstruction != nullptr && !_multiBuild){
                _buildList->clearSelection();
                _constructionFootprint = Texture();
                _selectedConstruction = nullptr;
            } else if (Container::getUseItem() != nullptr){
                Container::clearUseItem();
                _constructionFootprint = Texture();
            }

            break;

        case SV_ACTION_FINISHED:
            if (del != MSG_END)
                break;
            startAction(0); // Effectively, hide the cast bar.
            break;

        case SV_LOCATION:
        {
            std::string name;
            double x, y;
            singleMsg >> name >> del >> x >> del >> y >> del;
            if (del != MSG_END)
                break;
            Avatar *newUser = nullptr;
            const Point p(x, y);
            if (name == _username) {
                if (p.x == _character.location().x)
                    _pendingCharLoc.x = p.x;
                if (p.y == _character.location().y)
                    _pendingCharLoc.y = p.y;
                _character.destination(p);
                if (!_loaded) {
                    setEntityLocation(&_character, p);
                    _pendingCharLoc = p;
                }
                updateOffset();
                updateMapWindow();
                _connectionStatus = LOADED;
                _loaded = true;
                _tooltipNeedsRefresh = true;
                _mouseMoved = true;
            } else {
                if (_otherUsers.find(name) == _otherUsers.end()) {
                    // Create new Avatar
                    newUser = new Avatar(name, p);
                    _otherUsers[name] = newUser;
                    _entities.insert(newUser);
                }
                _otherUsers[name]->destination(p);
            }

            // Unwatch objects if out of range
            for (auto it = _objectsWatched.begin(); it != _objectsWatched.end(); ){
                ClientObject &obj = **it;
                ++it;
                if (distance(playerCollisionRect(), obj.collisionRect()) > ACTION_DISTANCE) {
                    obj.hideWindow();
                    unwatchObject(obj);
                }
            }

            // Forget about objects if out of cull range
            if (name == _username){ // No need to cull objects when other users move
                std::list<std::pair<size_t, Entity *> > objectsToRemove;
                for (auto pair : _objects)
                    if (outsideCullRange(pair.second->location(), CULL_HYSTERESIS_DISTANCE))
                        objectsToRemove.push_back(pair);
                for (auto pair : objectsToRemove){
                    if (pair.second == _currentMouseOverEntity)
                        _currentMouseOverEntity = nullptr;
                    if (pair.second == targetAsEntity())
                        clearTarget();
                    removeEntity(pair.second);
                    _objects.erase(_objects.find(pair.first));
                }
            }
            if (name == _username){ // We moved; look at everyone else
                std::list<Avatar*> usersToRemove;
                for (auto pair : _otherUsers)
                    if (outsideCullRange(pair.second->location(), CULL_HYSTERESIS_DISTANCE)){
                        _debug("Removing other user");
                        usersToRemove.push_back(pair.second);
                    }
                for (Avatar *avatar : usersToRemove){
                    if (_currentMouseOverEntity == avatar)
                        _currentMouseOverEntity = nullptr;
                    _otherUsers.erase(_otherUsers.find(avatar->name()));
                    removeEntity(avatar);
                }
            }

            break;
        }

        case SV_CLASS:
        {
            std::string username, className;
            singleMsg >> username >> del;
            readString(singleMsg, className, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            if (username == _username) {
                _character.setClass(className);
            } else {
                if (_otherUsers.find(username) == _otherUsers.end()) {
                    _debug("Class received for an unknown user.  Ignoring.", Color::FAILURE);
                    break;
                }
                _otherUsers[username]->setClass(className);
            }
            break;
        }

        case SV_GEAR:
        {
            std::string username, id;
            size_t slot;
            singleMsg >> username >> del >> slot >> del;
            readString(singleMsg, id, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            if (username == _username) {
                _debug("Own gear info received by wrong channel.  Ignoring.", Color::FAILURE);
                break;
            }
            if (_otherUsers.find(username) == _otherUsers.end()) {
                _debug("Gear received for an unknown user.  Ignoring.", Color::FAILURE);
                break;
            }

            // Handle empty id (item was unequipped)
            if (id == ""){
                _otherUsers[username]->gear()[slot].first = nullptr;
                break;
            }

            const auto it = _items.find(id);
            if (it == _items.end()){
                _debug << Color::FAILURE << "Unknown gear received (" << id << ").  Ignoring."
                       << Log::endl;
                break;
            }
            const ClientItem &item = it->second;
            if (item.gearSlot() >= GEAR_SLOTS){
                _debug("Gear info received for a non-gear item.  Ignoring.", Color::FAILURE);
                break;
            }
            _otherUsers[username]->gear()[slot].first = &item;
            break;
        }

        case SV_INVENTORY:
        {
            size_t serial, slot, quantity;
            std::string itemID;
            singleMsg >> serial >> del >> slot >> del >> itemID >> del >> quantity >> del;
            if (del != MSG_END)
                break;

            const ClientItem *item = nullptr;
            if (quantity > 0) {
                const auto it = _items.find(itemID);
                if (it == _items.end()) {
                    _debug << Color::FAILURE << "Unknown inventory item \"" << itemID
                           << "\"announced; ignored.";
                    break;
                }
                item = &it->second;
            }

            ClientItem::vect_t *container;
            ClientObject *object = nullptr;
            switch(serial){
                case INVENTORY: container = &_inventory;        break;
                case GEAR:      container = &_character.gear(); break;
                default:
                    auto it = _objects.find(serial);
                    if (it == _objects.end()) {
                        _debug("Received inventory of nonexistent object; ignored.", Color::FAILURE);
                        break;
                    }
                    object = it->second;
                    container = &object->container();
            }
            if (slot >= container->size()) {
                _debug("Received item in invalid inventory slot; ignored.", Color::FAILURE);
                break;
            }
            auto &invSlot = (*container)[slot];
            invSlot.first = item;
            invSlot.second = quantity;
            _recipeList->markChanged();
            switch(serial){
                case INVENTORY: _inventoryWindow->forceRefresh();   break;
                case GEAR:      _gearWindow->forceRefresh();        break;
                default:
                    object->onInventoryUpdate();
            }

            if (item != nullptr && // It's an actual item
                (serial == INVENTORY || serial == GEAR) && // You are receiving it
                _actionTimer > 0) // You were crafting or gathering
                    if (item->sounds() != nullptr)
                        item->sounds()->playOnce("drop");

            break;
        }

        case SV_OBJECT:
        {
            int serial;
            double x, y;
            std::string type;
            singleMsg >> serial >> del >> x >> del >> y >> del;
            readString(singleMsg, type, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;

            const ClientObjectType dummy(type);
            const Client::objectTypes_t::const_iterator typeIt = _objectTypes.find(&dummy);
            if (typeIt == _objectTypes.end()){
                _debug("Received object of invalid type; ignored.", Color::FAILURE);
                break;
            }
            const ClientObjectType *cot = *typeIt;

            std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it != _objects.end()){
                // Existing object: update its info.
                ClientObject &obj = *it->second;
                obj.location(Point(x, y));
                obj.type(cot);
                // Redraw window
                obj.assembleWindow(*this);
                obj.refreshTooltip();

            } else {
                // A new object was added; add entity to list
                ClientObject *obj;
                switch (cot->classTag()){
                case 'n':
                {
                    const ClientNPCType *npcType = static_cast<const ClientNPCType *>(cot);
                    obj = new ClientNPC(serial, npcType, Point(x, y));
                    break;
                }
                case 'v':
                {
                    const ClientVehicleType *vehicleType =
                            static_cast<const ClientVehicleType *>(cot);
                    obj = new ClientVehicle(serial, vehicleType, Point(x, y));
                    break;
                }
                case 'o':
                default:
                    obj = new ClientObject(serial, cot, Point(x, y));
                }
                _entities.insert(obj);
                _objects[serial] = obj;
            }
            break;
        }

        case SV_OBJECT_LOCATION:
        {
            int serial;
            double x, y;
            singleMsg >> serial >> del >> x >> del >> y >> del;
            if (del != MSG_END)
                break;
            std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()) {
                _debug("Server removed an object we didn't know about.", Color::WARNING);
                break; // We didn't know about this object
            }
            it->second->location(Point(x, y));
            break;
        }

        case SV_REMOVE_OBJECT:
        case SV_OBJECT_OUT_OF_RANGE:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::const_iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Server removed an object we didn't know about.", Color::WARNING);
                break; // We didn't know about this object
            }
            if (it->second == _currentMouseOverEntity)
                _currentMouseOverEntity = nullptr;
            if (it->second == targetAsEntity())
                clearTarget();
            removeEntity(it->second);
            _objects.erase(it);
            break;
        }

        case SV_OWNER:
        {
            int serial;
            std::string name;
            singleMsg >> serial >> del;
            readString(singleMsg, name, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received ownership info for an unknown object.", Color::FAILURE);
                break;
            }
            (it->second)->owner(name);
            (it->second)->refreshTooltip();
            break;
        }

        case SV_GATHERING_OBJECT:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received info about an unknown object.", Color::FAILURE);
                break;
            }
            (it->second)->beingGathered(true);
            break;
        }

        case SV_NOT_GATHERING_OBJECT:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received info about an unknown object.", Color::FAILURE);
                break;
            }
            (it->second)->beingGathered(false);
            break;
        }
        
        case SV_RECIPES:
        case SV_NEW_RECIPES:
        {
            int n;
            singleMsg >> n >> del;
            for (size_t i = 0; i != n; ++i){
                std::string recipe;
                readString(singleMsg, recipe, i == n - 1 ? MSG_END : MSG_DELIM);
                singleMsg >> del;
                _knownRecipes.insert(recipe);
            }
            if (msgCode == SV_NEW_RECIPES){
                _debug << "You have discovered ";
                if (n == 1)
                    _debug << "a new recipe";
                else
                    _debug << n << " new recipes";
                _debug << "!" << Log::endl;
            }

            _recipeList->markChanged();
            populateFilters();
            break;
        }
        
        case SV_CONSTRUCTIONS:
        case SV_NEW_CONSTRUCTIONS:
        {
            int n;
            singleMsg >> n >> del;
            for (size_t i = 0; i != n; ++i){
                std::string recipe;
                readString(singleMsg, recipe, i == n - 1 ? MSG_END : MSG_DELIM);
                singleMsg >> del;
                _knownConstructions.insert(recipe);
            }
            if (msgCode == SV_NEW_CONSTRUCTIONS){
                _debug << "You have discovered ";
                if (n == 1)
                    _debug << "a new construction";
                else
                    _debug << n << " new constructions";
                _debug << "!" << Log::endl;
            }
            populateBuildList();
            break;
        }

        case SV_NPC_HEALTH:
        {
            size_t serial;
            health_t health;
            singleMsg >> serial >> del >> health >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received health info for an unknown object.", Color::FAILURE);
                break;
            }
            if (it->second->classTag() != 'n'){
                _debug("Received health info for a non-NPC object.", Color::FAILURE);
            }
            ClientNPC &npc = dynamic_cast<ClientNPC &>(*it->second);
            npc.health(health);
            if (targetAsEntity() == &npc){
                _target.updateHealth(health);
                if (health == 0)
                    _target.makePassive();
            }
            npc.refreshTooltip();
            break;
        }

        case SV_PLAYER_HIT_NPC:
        {
            std::string username;
            int serial;
            readString(singleMsg, username, MSG_DELIM);
            singleMsg >> del >> serial >> del;
            if (del != MSG_END)
                break;
            auto objIt = _objects.find(serial);
            if (objIt == _objects.end()){
                _debug("Received combat info for an unknown object.", Color::FAILURE);
                break;
            }
            const ClientNPC &defender = * dynamic_cast<const ClientNPC *>(objIt->second);
            const Avatar *attacker = nullptr;
            if (username == _username)
                attacker = &character();
            else{
                auto userIt = _otherUsers.find(username);
                if (userIt == _otherUsers.end()){
                    _debug("Received combat info for an unknown player.", Color::FAILURE);
                    break;
                }
                attacker = userIt->second;
            }
            const SoundProfile *sounds = defender.npcType()->sounds();
            if (sounds != nullptr){
                if (defender.health() == 0)
                    sounds->playOnce("death");
                else
                    sounds->playOnce("defend");
            }
            attacker->playAttackSound();
            addParticles("combatDamage", defender.location());
            break;
        }

        case SV_NPC_HIT_PLAYER:
        {
            int serial;
            std::string username;
            singleMsg >> serial >> del;
            readString(singleMsg, username, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            auto objIt = _objects.find(serial);
            if (objIt == _objects.end()){
                _debug("Received combat info for an unknown object.", Color::FAILURE);
                break;
            }
            const ClientNPC &attacker = * dynamic_cast<const ClientNPC *>(objIt->second);
            const Avatar *defender = nullptr;
            if (username == _username)
                defender = &character();
            else{
                auto userIt = _otherUsers.find(username);
                if (userIt == _otherUsers.end()){
                    _debug("Received combat info for an unknown player.", Color::FAILURE);
                    break;
                }
                defender = userIt->second;
            }
            if (attacker.npcType()->sounds() != nullptr)
                attacker.npcType()->sounds()->playOnce("attack");
            defender->playDefendSound();
            addParticles("combatDamage", defender->location());
            break;
        }

        case SV_PLAYER_HIT_PLAYER:
        {
            std::string attackerName, defenderName;
            readString(singleMsg, attackerName);
            singleMsg >> del;
            readString(singleMsg, defenderName, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;

            const Avatar *attacker = nullptr;
            if (attackerName == _username)
                attacker = &character();
            else{
                auto userIt = _otherUsers.find(attackerName);
                if (userIt == _otherUsers.end()){
                    _debug("Received combat info for an unknown attacking player.", Color::FAILURE);
                    break;
                }
                attacker = userIt->second;
            }

            const Avatar *defender = nullptr;
            if (defenderName == _username)
                defender = &character();
            else{
                auto userIt = _otherUsers.find(defenderName);
                if (userIt == _otherUsers.end()){
                    _debug("Received combat info for an unknown defending player.", Color::FAILURE);
                    break;
                }
                defender = userIt->second;
            }
            
            attacker->playAttackSound();
            defender->playDefendSound();
            addParticles("combatDamage", defender->location());
            break;
        }

        case SV_LOOTABLE:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received loot info for an unknown object.", Color::FAILURE);
                break;
            }
            if (it->second->classTag() != 'n'){
                _debug("Received loot info for a non-NPC object.", Color::FAILURE);
                break;
            }
            ClientNPC &npc = dynamic_cast<ClientNPC &>(*it->second);
            npc.lootable(true);
            npc.refreshTooltip();
            break;
        }

        case SV_NOT_LOOTABLE:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received loot info for an unknown object.", Color::FAILURE);
                break;
            }
            if (it->second->classTag() != 'n'){
                _debug("Received loot info for a non-NPC object.", Color::FAILURE);
                break;
            }
            ClientNPC &npc = dynamic_cast<ClientNPC &>(*it->second);
            npc.lootable(false);
            npc.refreshTooltip();
            npc.hideWindow();
            break;
        }

        case SV_CONSTRUCTION_MATERIALS:
        {
            int serial, n;
            ItemSet set;
            std::string temp = singleMsg.str();
            singleMsg >> serial >> del >> n >> del;
            auto it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received construction-material info for unknown object");
                break;
            }
            ClientObject &obj = *it->second;
            for (size_t i = 0; i != n; ++i){
                std::string id;
                readString(singleMsg, id);
                const auto it = _items.find(id);
                if (it == _items.end()){
                    _debug("Received invalid construction-material info.", Color::FAILURE);
                    break;
                }
                size_t qty;
                singleMsg >> del >> qty >> del;
                const ClientItem *item = &it->second;
                set.add(item, qty);
            }
            obj.constructionMaterials(set);
            obj.assembleWindow(*this);
            obj.refreshTooltip();
            break;
        }

        case SV_HEALTH:
        {
            unsigned health;
            singleMsg >> health >> del;
            if (del != MSG_END)
                break;
            if (health < _health)
                addParticles("combatDamage", _character.location());
            _health = health;
            break;
        }

        case SV_MOUNTED:
        {
            size_t serial;
            std::string user;
            singleMsg >> serial >> del;
            readString(singleMsg, user, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            Avatar *userP = nullptr;
            if (user == _username) {
                userP = &_character;
                /*
                Cancel any requested movement; assumes that this message was preceded by
                SV_LOCATION, sending us on top of the vehicle.
                */
                _pendingCharLoc = _character.destination();
            }else{
                auto it = _otherUsers.find(user);
                if (it == _otherUsers.end())
                    _debug("Received vehicle info for an unknown user", Color::FAILURE);
                else
                    userP = it->second;
            }
            userP->driving(true);
            auto pairIt = _objects.find(serial);
            if (pairIt == _objects.end())
                _debug("Received driver info for an unknown vehicle", Color::FAILURE);
            else{
                ClientVehicle *v = dynamic_cast<ClientVehicle *>(pairIt->second);
                v->driver(userP);
            }
            break;
        }

        case SV_UNMOUNTED:
        {
            size_t serial;
            std::string user;
            singleMsg >> serial >> del;
            readString(singleMsg, user, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            Avatar *userP = nullptr;
            if (user == _username){
                userP = &_character;
                _isDismounting = false;
                /*
                Cancel any requested movement; assumes that this message was preceded by
                SV_LOCATION, sending us to the dismount location.
                */
                _pendingCharLoc = _character.destination();
            }else{
                auto it = _otherUsers.find(user);
                if (it == _otherUsers.end())
                    _debug("Received vehicle info for an unknown user", Color::FAILURE);
                else
                    userP = it->second;
            }
            userP->driving(false);
            auto pairIt = _objects.find(serial);
            if (pairIt == _objects.end())
                _debug("Received driver info for an unknown vehicle", Color::FAILURE);
            else{
                ClientVehicle *v = dynamic_cast<ClientVehicle *>(pairIt->second);
                v->driver(nullptr);
            }
            break;
        }

        case SV_STATS:
        {
            singleMsg >> _stats.health >> del >> _stats.attack >> del >> _stats.attackTime >> del
                      >> _stats.speed >> del;
            if (del != MSG_END)
                break;
            break;
        }

        case SV_MERCHANT_SLOT:
        {
            size_t serial, slot, wareQty, priceQty;
            std::string ware, price;
            singleMsg >> serial >> del >> slot >> del
                      >> ware >> del >> wareQty >> del
                      >> price >> del >> priceQty >> del;
            if (del != MSG_END)
                return;
            auto objIt = _objects.find(serial);
            if (objIt == _objects.end()){
                _debug("Info received about unknown object.", Color::FAILURE);
                break;
            }
            ClientObject &obj = const_cast<ClientObject &>(*objIt->second);
            size_t slots = obj.objectType()->merchantSlots();
            if (slot >= slots){
                _debug("Received invalid merchant slot.", Color::FAILURE);
                break;
            }
            if (ware.empty() || price.empty()){
                obj.setMerchantSlot(slot, ClientMerchantSlot());
                break;
            }
            auto wareIt = _items.find(ware);
            if (wareIt == _items.end()){
                _debug("Received merchant slot describing invalid item", Color::FAILURE);
                break;
            }
            const ClientItem *wareItem = &wareIt->second;
            auto priceIt = _items.find(price);
            if (priceIt == _items.end()){
                _debug("Received merchant slot describing invalid item", Color::FAILURE);
                break;
            }
            const ClientItem *priceItem = &priceIt->second;
            obj.setMerchantSlot(slot, ClientMerchantSlot(wareItem, wareQty, priceItem, priceQty));
            break;
        }

        case SV_TRANSFORM_TIME:
        {
            size_t serial, remaining;
            singleMsg >> serial >> del >> remaining >> del;
            if (del != MSG_END)
                return;
            auto objIt = _objects.find(serial);
            if (objIt == _objects.end()){
                _debug("Info received about unknown object.", Color::FAILURE);
                break;
            }
            ClientObject &obj = const_cast<ClientObject &>(*objIt->second);
            obj.transformTimer(remaining);
            break;
        }

        case SV_AT_WAR_WITH:
        {
            std::string username;
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            username = std::string(buffer);
            singleMsg >> del;
            if (del != MSG_END)
                return;

            _atWarWith.insert(username);
            _debug << "You are now at war with " << username << Log::endl;
            break;
        }

        case SV_SAY:
        {
            std::string username, message;
            singleMsg >> username >> del;
            readString(singleMsg, message, MSG_END);
            singleMsg >> del;
            if (del != MSG_END || username == _username) // We already know we said this.
                break;
            addChatMessage("[" + username + "] " + message, SAY_COLOR);
            break;
        }

        case SV_WHISPER:
        {
            std::string username, message;
            singleMsg >> username >> del;
            readString(singleMsg, message, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            addChatMessage("[From " + username + "] " + message, WHISPER_COLOR);
            _lastWhisperer = username;
            break;
        }

        default:
            _debug << Color::FAILURE << "Unhandled message: " << msg << Log::endl;
        }

        if (del != MSG_END && !iss.eof()) {
            _debug << Color::FAILURE << "Bad message ending. code=" << msgCode
                   << "; remaining message = " << del << singleMsg.str() << Log::endl;
        }

        iss.peek();
    }
}

void Client::sendRawMessage(const std::string &msg) const{
    _socket.sendMessage(msg);
}

void Client::sendMessage(MessageCode msgCode, const std::string &args) const{
    // Compile message
    std::ostringstream oss;
    oss << MSG_START << msgCode;
    if (args != "")
        oss << MSG_DELIM << args;
    oss << MSG_END;

    // Send message
    sendRawMessage(oss.str());
}

void Client::initializeMessageNames(){    
    _messageCommands["location"] = CL_LOCATION;
    _messageCommands["cancel"] = CL_CANCEL_ACTION;
    _messageCommands["craft"] = CL_CRAFT;
    _messageCommands["constructItem"] = CL_CONSTRUCT_ITEM;
    _messageCommands["construct"] = CL_CONSTRUCT;
    _messageCommands["gather"] = CL_GATHER;
    _messageCommands["deconstruct"] = CL_DECONSTRUCT;
    _messageCommands["drop"] = CL_DROP;
    _messageCommands["swap"] = CL_SWAP_ITEMS;
    _messageCommands["trade"] = CL_TRADE;
    _messageCommands["setMerchantSlot"] = CL_SET_MERCHANT_SLOT;
    _messageCommands["clearMerchantSlot"] = CL_CLEAR_MERCHANT_SLOT;
    _messageCommands["startWatching"] = CL_START_WATCHING;
    _messageCommands["stopWatching"] = CL_STOP_WATCHING;
    _messageCommands["targetNPC"] = CL_TARGET_NPC;
    _messageCommands["targetPlayer"] = CL_TARGET_PLAYER;
    _messageCommands["take"] = CL_TAKE_ITEM;
    _messageCommands["mount"] = CL_MOUNT;
    _messageCommands["dismount"] = CL_DISMOUNT;
    _messageCommands["war"] = CL_DECLARE_WAR;

    _messageCommands["say"] = CL_SAY;
    _messageCommands["s"] = CL_SAY;
    _messageCommands["whisper"] = CL_WHISPER;
    _messageCommands["w"] = CL_WHISPER;
    
    _messageCommands["give"] = DG_GIVE;
    _messageCommands["unlock"] = DG_UNLOCK;

    _errorMessages[SV_TOO_FAR] = "You are too far away to perform that action.";
    _errorMessages[SV_DOESNT_EXIST] = "That object doesn't exist.";
    _errorMessages[SV_INVENTORY_FULL] = "Your inventory is full.";
    _errorMessages[SV_NEED_MATERIALS] = "You do not have the materials neded to create that item.";
    _errorMessages[SV_NEED_TOOLS] = "You do not have the tools needed to create that item.";
    _errorMessages[SV_ACTION_INTERRUPTED] = "Action interrupted.";
    _errorMessages[SV_SERVER_FULL] = "The server is full.  Attempting reconnection...";
    _errorMessages[SV_INVALID_USER] = "That user doesn't exist.";
    _errorMessages[SV_INVALID_ITEM] = "That is not a real item.";
    _errorMessages[SV_CANNOT_CRAFT] = "That item cannot be crafted.";
    _errorMessages[SV_EMPTY_SLOT] = "That inventory slot is empty.";
    _errorMessages[SV_INVALID_SLOT] = "That is not a valid inventory slot.";
    _errorMessages[SV_CANNOT_CONSTRUCT] = "That item cannot be constructed.";
    _errorMessages[SV_BLOCKED] = "That location is unsuitable for that action.";
    _errorMessages[SV_NO_PERMISSION] = "You do not have permission to do that.";
    _errorMessages[SV_NOT_MERCHANT] = "That is not a merchant object.";
    _errorMessages[SV_INVALID_MERCHANT_SLOT] = "That is not a valid merchant slot.";
    _errorMessages[SV_NO_WARE] = "The object does not have enough items in stock.";
    _errorMessages[SV_NO_PRICE] = "You cannot afford to buy that.";
    _errorMessages[SV_MERCHANT_INVENTORY_FULL] =
        "The object does not have enough inventory space for that exchange.";
    _errorMessages[SV_NOT_EMPTY] = "That object is not empty.";
    _errorMessages[SV_NOT_NPC] = "That object is not an NPC.";
    _errorMessages[SV_NPC_DEAD] = "That NPC is dead.";
    _errorMessages[SV_NPC_SWAP] = "You can't put items inside an NPC.";
    _errorMessages[SV_TAKE_SELF] = "You can't take an item from yourself.";
    _errorMessages[SV_NOT_GEAR] = "That item can't be used in that equipment slot.";
    _errorMessages[SV_NOT_VEHICLE] = "That isn't a vehicle.";
    _errorMessages[SV_VEHICLE_OCCUPIED] = "You can't do that to an occupied vehicle.";
    _errorMessages[SV_NO_VEHICLE] = "You are not in a vehicle.";
    _errorMessages[SV_UNKNOWN_RECIPE] = "You don't know that recipe.";
    _errorMessages[SV_UNKNOWN_CONSTRUCTION] = "You don't know how to construct that object.";
    _errorMessages[SV_WRONG_MATERIAL] = "The construction site doesn't need that.";
    _errorMessages[SV_UNDER_CONSTRUCTION] = "That object is still under construction.";
    _errorMessages[SV_AT_PEACE] = "You are not at war with that player.";
}

void Client::performCommand(const std::string &commandString){
    std::istringstream iss(commandString);
    std::string token;
    char c;
    iss >> c;
    if (c != '/') {
        assert(false);
        _debug("Commands must begin with '/'.", Color::FAILURE);
        return;
    }

    static char buffer[BUFFER_SIZE+1];
    iss.get(buffer, BUFFER_SIZE, ' ');
    std::string command(buffer);

    //std::vector<std::string> args;
    //while (!iss.eof()) {
    //    while (iss.peek() == ' ')
    //        iss.ignore(BUFFER_SIZE, ' ');
    //    if (iss.eof())
    //        break;
    //    iss.get(buffer, BUFFER_SIZE, ' ');
    //    args.push_back(buffer);
    //}

    // Messages to server
    if (_messageCommands.find(command) != _messageCommands.end()){
        MessageCode code = static_cast<MessageCode>(_messageCommands[command]);
        std::string argsString;
        switch(code){

        case CL_SAY:
            iss.get(buffer, BUFFER_SIZE);
            argsString = buffer;
            addChatMessage("[" + _username + "] " + argsString, SAY_COLOR);
            break;

        case CL_WHISPER:
        {
            while (iss.peek() == ' ') iss.ignore(BUFFER_SIZE, ' ');
            if (iss.eof()) break;
            iss.get(buffer, BUFFER_SIZE, ' ');
            std::string username(buffer);
            iss.get(buffer, BUFFER_SIZE);
            argsString = username + MSG_DELIM + buffer;
            addChatMessage("[To " +username + "] " + buffer, WHISPER_COLOR);
            break;
        }

        default:
            std::vector<std::string> args;
            while (!iss.eof()) {
                while (iss.peek() == ' ')
                    iss.ignore(BUFFER_SIZE, ' ');
                if (iss.eof())
                    break;
                iss.get(buffer, BUFFER_SIZE, ' ');
                args.push_back(buffer);
            }
            std::ostringstream oss;
            for (size_t i = 0; i != args.size(); ++i){
                oss << args[i];
                if (i < args.size() - 1) {
                    if (code == CL_SAY || // Allow spaces in messages
                        code == CL_WHISPER && i > 0)
                        oss << ' ';
                    else
                        oss << MSG_DELIM;
                }
            }
            argsString = oss.str();
        }

        sendMessage(code, argsString);
        return;
    }

    _debug << Color::FAILURE << "Unknown command: " << command << Log::endl;
}

void Client::sendClearTargetMessage() const{
    sendMessage(CL_TARGET_NPC, makeArgs(0));
}
