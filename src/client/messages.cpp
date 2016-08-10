// (C) 2016 Tim Gurto

#include <cassert>
#include "Client.h"

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
            _debug << Color::RED << "Malformed message; discarded \""
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
        Color errorMessageColor = Color::RED;

        switch(msgCode) {

        case SV_WELCOME:
        {
            if (del != MSG_END)
                break;
            _loggedIn = true;
            _timeSinceConnectAttempt = 0;
            _lastPingSent = _lastPingReply = _time;
            _debug("Successfully logged in to server", Color::GREEN);
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
            _debug << Color::RED << "The user " << _username
                   << " is already connected to the server." << Log::endl;
            break;

        case SV_INVALID_USERNAME:
            if (del != MSG_END)
                break;
            _invalidUsername = true;
            _debug << Color::RED << "The username " << _username << " is invalid." << Log::endl;
            break;

        case SV_SERVER_FULL:
            _socket = Socket();
            _loggedIn = false;
            _debug(_errorMessages[msgCode], Color::RED);
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
            errorMessageColor = Color::YELLOW; // Yellow above, red below
        case SV_INVALID_USER:
        case SV_INVALID_ITEM:
        case SV_CANNOT_CRAFT:
        case SV_INVALID_SLOT:
        case SV_EMPTY_SLOT:
        case SV_CANNOT_CONSTRUCT:
        case SV_NOT_MERCHANT:
        case SV_INVALID_MERCHANT_SLOT:
            if (del != MSG_END)
                break;
            _debug(_errorMessages[msgCode], errorMessageColor);
            startAction(0);
            break;

        case SV_ITEM_NEEDED:
        {
            std::string reqItemClass;
            readString(singleMsg, reqItemClass, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            std::string msg = "You need a";
            const char first = reqItemClass.front();
            if (first == 'a' || first == 'e' || first == 'i' ||
                first == 'o' || first == 'u')
                msg += 'n';
            _debug(msg + ' ' + reqItemClass + " to do that.", Color::YELLOW);
            startAction(0);
            break;
        }

        case SV_ACTION_STARTED:
            ms_t time;
            singleMsg >> time >> del;
            if (del != MSG_END)
                break;
            startAction(time);

            // If crafting, hide footprint now that it has successfully started.
            Container::clearUseItem();
            _constructionFootprint = Texture();

            break;

        case SV_ACTION_FINISHED:
            if (del != MSG_END)
                break;
            startAction(0); // Effectively, hide the cast bar.
            break;

        case SV_MAP_SIZE:
        {
            size_t x, y;
            singleMsg >> x >> del >> y >> del;
            if (del != MSG_END)
                break;
            _mapX = x;
            _mapY = y;
            _map = std::vector<std::vector<size_t> >(_mapX);
            for (size_t x = 0; x != _mapX; ++x)
                _map[x] = std::vector<size_t>(_mapY, 0);
            break;
        }

        case SV_TERRAIN:
        {
            size_t x, y, n;
            singleMsg >> x >> del >> y >> del >> n >> del;
            if (x + n > _mapX)
                break;
            if (y > _mapY)
                break;
            std::vector<size_t> terrain;
            for (size_t i = 0; i != n; ++i) {
                size_t index;
                singleMsg >> index >> del;
                if (index > _terrain.size()) {
                    _debug << Color::RED << "Invalid terrain type receved ("
                           << index << "); aborted." << Log::endl;
                    break;
                }
                terrain.push_back(index);
            }
            if (del != MSG_END)
                break;
            if (terrain.size() != n)
                break;
            for (size_t i = 0; i != n; ++i)
                _map[x+i][y] = terrain[i];
            break;
        }

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

        case SV_INVENTORY:
        {
            size_t serial, slot, quantity;
            std::string itemID;
            singleMsg >> serial >> del >> slot >> del >> itemID >> del >> quantity >> del;
            if (del != MSG_END)
                break;

            const Item *item = nullptr;
            if (quantity > 0) {
                std::set<Item>::const_iterator it = _items.find(itemID);
                if (it == _items.end()) {
                    _debug << Color::RED << "Unknown inventory item \"" << itemID
                           << "\"announced; ignored.";
                    break;
                }
                item = &*it;
            }

            Item::vect_t *container;
            ClientObject *object = nullptr;
            if (serial == 0)
                container = &_inventory;
            else {
                auto it = _objects.find(serial);
                if (it == _objects.end()) {
                    _debug("Received inventory of nonexistent object; ignored.", Color::RED);
                    break;
                }
                object = it->second;
                container = &object->container();
            }
            if (slot >= container->size()) {
                _debug("Received item in invalid inventory slot; ignored.", Color::RED);
                break;
            }
            auto &invSlot = (*container)[slot];
            invSlot.first = item;
            invSlot.second = quantity;
            _recipeList->markChanged();
            if (serial == 0)
                _inventoryWindow->forceRefresh();
            else
                object->refreshWindow();
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
            std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()) {
                // A new object was added; add entity to list
                const std::set<ClientObjectType>::const_iterator it = _objectTypes.find(type);
                if (it == _objectTypes.end())
                    break;
                ClientObject *const obj = new ClientObject(serial, &*it, Point(x, y));
                _entities.insert(obj);
                _objects[serial] = obj;
            }
            break;
        }

        case SV_REMOVE_OBJECT:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::const_iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Server removed an object we didn't know about.", Color::YELLOW);
                break; // We didn't know about this object
            }
            if (it->second == _currentMouseOverEntity)
                _currentMouseOverEntity = nullptr;
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
                _debug("Received ownership info for an unknown object.", Color::RED);
                break;
            }
            (it->second)->owner(name);
            break;
        }

        case SV_HEALTH:
        {
            unsigned health;
            singleMsg >> health >> del;
            if (del != MSG_END)
                break;
            _health = health;
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
                _debug("Info received about unknown object.", Color::RED);
                break;
            }
            ClientObject &obj = const_cast<ClientObject &>(*objIt->second);
            size_t slots = obj.objectType()->merchantSlots();
            if (slot >= slots){
                _debug("Received invalid merchant slot.", Color::RED);
                break;
            }
            if (ware.empty() || price.empty()){
                obj.setMerchantSlot(slot, MerchantSlot());
                break;
            }
            auto wareIt = _items.find(ware);
            if (wareIt == _items.end()){
                _debug("Received merchant slot describing invalid item", Color::RED);
                break;
            }
            auto priceIt = _items.find(price);
            if (priceIt == _items.end()){
                _debug("Received merchant slot describing invalid item", Color::RED);
                break;
            }
            obj.setMerchantSlot(slot, MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty));
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
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }

        if (del != MSG_END && !iss.eof()) {
            _debug << Color::RED << "Bad message ending. code=" << msgCode
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

    _messageCommands["say"] = CL_SAY;
    _messageCommands["s"] = CL_SAY;
    _messageCommands["whisper"] = CL_WHISPER;
    _messageCommands["w"] = CL_WHISPER;

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
    _errorMessages[SV_BLOCKED] = "That location is already occupied.";
    _errorMessages[SV_NO_PERMISSION] = "You do not have permission to do that.";
    _errorMessages[SV_NOT_MERCHANT] = "That is not a merchant object.";
    _errorMessages[SV_INVALID_MERCHANT_SLOT] = "That is not a valid merchant slot.";
    _errorMessages[SV_NO_WARE] = "The object does not have enough items in stock.";
    _errorMessages[SV_NO_PRICE] = "You cannot afford to buy that.";
    _errorMessages[SV_MERCHANT_INVENTORY_FULL] =
        "The object does not have enough inventory space for that exchange.";
    _errorMessages[SV_NOT_EMPTY] = "That object is not empty.";
}

void Client::performCommand(const std::string &commandString){
    std::istringstream iss(commandString);
    std::string token;
    char c;
    iss >> c;
    if (c != '/') {
        assert(false);
        _debug("Commands must begin with '/'.", Color::RED);
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

    _debug("Unknown command.", Color::RED);
}
