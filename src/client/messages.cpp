// (C) 2016 Tim Gurto

#include <cassert>
#include "Client.h"

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
            Uint32 timeSent;
            singleMsg >> timeSent >> del;
            if (del != MSG_END)
                break;
            _lastPingReply = _time;
            _latency = (_time - timeSent) / 2;
            break;
        }

        case SV_USER_DISCONNECTED:
        {
            std::string name;
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            name = std::string(buffer);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            const std::map<std::string, Avatar*>::iterator it = _otherUsers.find(name);
            if (it != _otherUsers.end()) {
                removeEntity(it->second);
                _otherUsers.erase(it);
            }
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
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            reqItemClass = std::string(buffer);
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
            Uint32 time;
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
            singleMsg.get(buffer, BUFFER_SIZE, MSG_DELIM);
            name = std::string(buffer);
            singleMsg >> del >> x >> del >> y >> del;
            if (del != MSG_END)
                break;
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
                    Avatar *newUser = new Avatar(name, p);
                    _otherUsers[name] = newUser;
                    _entities.insert(newUser);
                }
                _otherUsers[name]->destination(p);
            }

            // Unwatch objects if out of range
            for (const ClientObject *obj : _objectsWatched){
                if (distance(playerCollisionRect(), obj->collisionRect()) > ACTION_DISTANCE)
                    unwatchObject(*obj);
            }

            break;
        }

        case SV_INVENTORY:
        {
            size_t serial, slot, quantity;
            std::string itemID;
            singleMsg >> serial >> del >> slot >> del;
            singleMsg.get(buffer, BUFFER_SIZE, MSG_DELIM);
            itemID = std::string(buffer);
            singleMsg >> del >> quantity >> del;
            if (del != MSG_END)
                break;

            const Item *item = 0;
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
            ClientObject *object = 0;
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
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            type = std::string(buffer);
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
                assert(false);
                break; // We didn't know about this object
            }
            if (it->second == _currentMouseOverEntity)
                _currentMouseOverEntity = 0;
            removeEntity(it->second);
            _objects.erase(it);
            break;
        }

        case SV_OWNER:
        {
            int serial;
            singleMsg >> serial >> del;
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            singleMsg >> del;
            if (del != MSG_END)
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received ownership info for an unknown object.", Color::RED);
                break;
            }
            (it->second)->owner(std::string(buffer));
            break;
        }

        case SV_SAY:
        {
            singleMsg.get(buffer, BUFFER_SIZE, MSG_DELIM);
            std::string username(buffer);
            singleMsg >> del;
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            std::string message(buffer);
            singleMsg >> del;
            if (del != MSG_END || username == _username) // We already know we said this.
                break;
            addChatMessage("[" + username + "] " + message, SAY_COLOR);
            break;
        }

        case SV_WHISPER:
        {
            singleMsg.get(buffer, BUFFER_SIZE, MSG_DELIM);
            std::string username(buffer);
            singleMsg >> del;
            singleMsg.get(buffer, BUFFER_SIZE, MSG_END);
            std::string message(buffer);
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
