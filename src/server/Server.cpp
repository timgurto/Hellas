// (C) 2015 Tim Gurto

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#ifndef SINGLE_THREAD
#include <mutex>
#include <thread>
#endif

#include "ItemSet.h"
#include "Object.h"
#include "ObjectType.h"
#include "Recipe.h"
#include "Server.h"
#include "User.h"
#include "../Socket.h"
#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "../messageCodes.h"
#include "../util.h"

extern Args cmdLineArgs;

Server *Server::_instance = 0;
LogConsole *Server::_debugInstance = 0;

const int Server::MAX_CLIENTS = 20;
const size_t Server::BUFFER_SIZE = 1023;

const Uint32 Server::CLIENT_TIMEOUT = 10000;
const Uint32 Server::MAX_TIME_BETWEEN_LOCATION_UPDATES = 300;

const Uint32 Server::SAVE_FREQUENCY = 1000;

const double Server::MOVEMENT_SPEED = 80;
const int Server::ACTION_DISTANCE = 30;

const int Server::TILE_W = 16;
const int Server::TILE_H = 16;

Server::Server():
_time(SDL_GetTicks()),
_lastTime(_time),
_socket(),
_loop(true),
_mapX(0),
_mapY(0),
_debug("server.log"),
_lastSave(_time){
    _instance = this;
    _debugInstance = &_debug;

    _debug << cmdLineArgs << Log::endl;
    Socket::debug = &_debug;

    loadData();

    _debug("Server initialized");

    // Socket details
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    _socket.bind(serverAddr);
    _debug << "Server address: " << inet_ntoa(serverAddr.sin_addr)
           << ":" << ntohs(serverAddr.sin_port) << Log::endl;
    _socket.listen();

    _debug("Listening for connections");
}

Server::~Server(){
    saveData(_objects);
}

void Server::checkSockets(){
    // Populate socket list with active sockets
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    for (const Socket &socket : _clientSockets) {
        FD_SET(socket.getRaw(), &readFDs);
    }

    // Poll for activity
    static const timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }
    _time = SDL_GetTicks();

    // Activity on server socket: new connection
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {

        if (_clientSockets.size() == MAX_CLIENTS) {
            _debug("No room for additional clients; all slots full");
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr,
                                       (int*)&Socket::sockAddrSize);
            Socket s(tempSocket);
            // Allow time for rejection message to be sent before closing socket
            s.delayClosing(5000);
            sendMessage(s, SV_SERVER_FULL);
        } else {
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr,
                                       (int*)&Socket::sockAddrSize);
            if (tempSocket == SOCKET_ERROR) {
                _debug << Color::RED << "Error accepting connection: "
                       << WSAGetLastError() << Log::endl;
            } else {
                _debug << Color::GREEN << "Connection accepted: "
                       << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port)
                       << ", socket number = " << tempSocket << Log::endl;
                _clientSockets.insert(tempSocket);
            }
        }
    }

    // Activity on client socket: message received or client disconnected
    for (std::set<Socket>::iterator it = _clientSockets.begin(); it != _clientSockets.end();) {
        SOCKET raw = it->getRaw();
        if (FD_ISSET(raw, &readFDs)) {
            sockaddr_in clientAddr;
            getpeername(raw, (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            static char buffer[BUFFER_SIZE+1];
            const int charsRead = recv(raw, buffer, BUFFER_SIZE, 0);
            if (charsRead == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAECONNRESET) {
                    // Client disconnected
                    _debug << "Client " << raw << " disconnected" << Log::endl;
                    removeUser(raw);
                    closesocket(raw);
                    _clientSockets.erase(it++);
                    continue;
                } else {
                    _debug << Color::RED << "Error receiving message: " << err << Log::endl;
                }
            } else if (charsRead == 0) {
                // Client disconnected
                _debug << "Client " << raw << " disconnected" << Log::endl;
                removeUser(*it);
                closesocket(raw);
                _clientSockets.erase(it++);
                continue;
            } else {
                // Message received
                buffer[charsRead] = '\0';
                _messages.push(std::make_pair(*it, std::string(buffer)));
            }
        }
        ++it;
    }
}

void Server::run(){
    while (_loop) {
        _time = SDL_GetTicks();
        const Uint32 timeElapsed = _time - _lastTime;
        _lastTime = _time;

        // Check that clients are alive
        for (std::set<User>::iterator it = _users.begin(); it != _users.end();) {
            if (!it->alive()) {
                _debug << Color::RED << "User " << it->name() << " has timed out." << Log::endl;
                std::set<User>::iterator next = it; ++next;
                removeUser(it);
                it = next;
            } else {
                ++it;
            }
        }

        // Save data
        if (_time - _lastSave >= SAVE_FREQUENCY) {
            for (const User &user : _users) {
                writeUserData(user);
            }
#ifdef SINGLE_THREAD
            saveData(_objects);
#else
            std::thread(saveData, _objects).detach();
#endif
            _lastSave = _time;
        }

        // Update users
        for (const User &user : _users)
            const_cast<User&>(user).update(timeElapsed);

        // Deal with any messages from the server
        while (!_messages.empty()){
            handleMessage(_messages.front().first, _messages.front().second);
            _messages.pop();
        }

        checkSockets();

        SDL_Delay(10);
    }

    // Save all user data
    for(const User &user : _users){
        writeUserData(user);
    }
}

void Server::addUser(const Socket &socket, const std::string &name){
    User newUser(name, 0, socket);
    const bool userExisted = readUserData(newUser);
    if (!userExisted) {
        do {
            newUser.location(mapRand());
        } while (!isLocationValid(newUser.location(), User::OBJECT_TYPE));
        _debug << "New";
    } else {
        _debug << "Existing";
    }
    _debug << " user, " << name << " has logged in." << Log::endl;
    _usernames.insert(name);

    // Send welcome message
    sendMessage(socket, SV_WELCOME);

    // Send new user the map
    sendMessage(newUser.socket(), SV_MAP_SIZE, makeArgs(_mapX, _mapY));
    for (size_t y = 0; y != _mapY; ++y){
        std::ostringstream oss;
        oss << "0," << y << "," << _mapX;
        for (size_t x = 0; x != _mapX; ++x)
            oss << "," << _map[x][y];
        sendMessage(newUser.socket(), SV_TERRAIN, oss.str());
    }

    // Send him everybody else's location
    for (const User &user : _users)
        sendMessage(newUser.socket(), SV_LOCATION, user.makeLocationCommand());

    // Send him object details
    for (const Object &obj : _objects) {
        assert (obj.type());
        sendMessage(newUser.socket(), SV_OBJECT,
                    makeArgs(obj.serial(), obj.location().x, obj.location().y, obj.type()->id()));
        if (!obj.owner().empty())
            sendMessage(newUser.socket(), SV_OWNER, makeArgs(obj.serial(), obj.owner()));
    }

    // Send him his inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        if (newUser.inventory(i).first)
            sendInventoryMessage(newUser, 0, i);
    }

    // Add new user to list, and broadcast his location
    _users.insert(newUser);
    broadcast(SV_LOCATION, newUser.makeLocationCommand());
}

void Server::removeUser(const std::set<User>::iterator &it){
        // Broadcast message
        broadcast(SV_USER_DISCONNECTED, it->name());

        // Save user data
        writeUserData(*it);

        _usernames.erase(it->name());
        _users.erase(it);
}

void Server::removeUser(const Socket &socket){
    const std::set<User>::iterator it = _users.find(socket);
    if (it != _users.end())
        removeUser(it);
}

bool Server::isValidObject(const Socket &client, const User &user,
                           const std::set<Object>::const_iterator &it) const{
    // Object doesn't exist
    if (it == _objects.end()) {
        sendMessage(client, SV_DOESNT_EXIST);
        return false;
    }
    
    // Check distance from user
    if (distance(user.collisionRect(), it->collisionRect()) > ACTION_DISTANCE) {
        sendMessage(client, SV_TOO_FAR);
        return false;
    }

    return true;
}

void Server::handleMessage(const Socket &client, const std::string &msg){
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    std::istringstream iss(msg);
    User *user = 0;
    while (iss.peek() == '[') {
        iss >> del >> msgCode >> del;
        
        // Discard message if this client has not yet sent CL_I_AM
        std::set<User>::iterator it = _users.find(client);
        if (it == _users.end() && msgCode != CL_I_AM) {
            continue;
        }
        if (msgCode != CL_I_AM) {
            User & userRef = const_cast<User&>(*it);
            user = &userRef;
            user->contact();
        }

        switch(msgCode) {

        case CL_PING:
        {
            Uint32 timeSent;
            iss >> timeSent  >> del;
            if (del != ']')
                return;
            sendMessage(user->socket(), SV_PING_REPLY, makeArgs(timeSent));
            break;
        }

        case CL_I_AM:
        {
            std::string name;
            iss.get(buffer, BUFFER_SIZE, ']');
            name = std::string(buffer);
            iss >> del;
            if (del != ']')
                return;

            // Check that username is valid
            bool invalid = false;
            for (char c : name){
                if (c < 'a' || c > 'z') {
                    sendMessage(client, SV_INVALID_USERNAME);
                    invalid = true;
                    break;
                }
            }
            if (invalid)
                break;

            // Check that user isn't already logged in
            if (_usernames.find(name) != _usernames.end()) {
                sendMessage(client, SV_DUPLICATE_USERNAME);
                invalid = true;
                break;
            }

            addUser(client, name);

            break;
        }

        case CL_LOCATION:
        {
            double x, y;
            iss >> x >> del >> y >> del;
            if (del != ']')
                return;
            user->cancelAction();
            user->updateLocation(Point(x, y));
            broadcast(SV_LOCATION, user->makeLocationCommand());
            break;
        }

        case CL_CRAFT:
        {
            std::string id;
            iss.get(buffer, BUFFER_SIZE, ']');
            id = std::string(buffer);
            iss >> del;
            if (del != ']')
                return;
            user->cancelAction();
            const std::set<Recipe>::const_iterator it = _recipes.find(id);
            if (it == _recipes.end()) {
                sendMessage(client, SV_INVALID_ITEM);
                break;
            }
            ItemSet remaining;
            if (!user->hasItems(it->materials())) {
                sendMessage(client, SV_NEED_MATERIALS);
                break;
            }
            if (!user->hasTools(it->tools())) {
                sendMessage(client, SV_NEED_TOOLS);
                break;
            }
            user->actionCraft(*it);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(it->time()));
            break;
        }

        case CL_CONSTRUCT:
        {
            size_t slot;
            double x, y;
            iss >> slot >> del >> x >> del >> y >> del;
            if (del != ']')
                return;
            user->cancelAction();
            if (slot >= User::INVENTORY_SIZE) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }
            const std::pair<const Item *, size_t> &invSlot = user->inventory(slot);
            if (!invSlot.first) {
                sendMessage(client, SV_EMPTY_SLOT);
                break;
            }
            const Item &item = *invSlot.first;
            if (!item.constructsObject()) {
                sendMessage(client, SV_CANNOT_CONSTRUCT);
                break;
            }
            const Point location(x, y);
            const ObjectType &objType = *item.constructsObject();
            if (distance(user->collisionRect(), objType.collisionRect() + location) >
                ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
                break;
            }
            if (!isLocationValid(location, objType)) {
                sendMessage(client, SV_BLOCKED);
                break;
            }
            user->actionConstruct(objType, location, slot);
            sendMessage(client, SV_ACTION_STARTED,
                        makeArgs(objType.constructionTime()));
            break;
        }

        case CL_CANCEL_ACTION:
        {
            iss >> del;
            if (del != ']')
                return;
            user->cancelAction();
            break;
        }

        case CL_GATHER:
        {
            int serial;
            iss >> serial >> del;
            if (del != ']')
                return;
            user->cancelAction();
            std::set<Object>::const_iterator it = _objects.find(serial);
            if (!isValidObject(client, *user, it))
                break;
            const Object &obj = *it;
            // Check that the user meets the requirements
            assert (obj.type());
            const std::string &gatherReq = obj.type()->gatherReq();
            if (gatherReq != "none" && !user->hasTool(gatherReq)) {
                sendMessage(client, SV_ITEM_NEEDED, gatherReq);
                break;
            }
            user->actionTarget(&obj);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(obj.type()->actionTime()));
            break;
        }

        case CL_DROP:
        {
            size_t serial, slot;
            iss >> serial >> slot >> del;
            if (del != ']')
                return;
            Item::vect_t *container;
            if (serial == 0)
                container = &user->inventory();
            else {
                auto it = _objects.find(serial);
                if (!isValidObject(client, *user, it))
                    break;
                container = &(const_cast<Object&>(*it).container());
            }

            if (slot >= container->size()) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }
            auto &containerSlot = (*container)[slot];
            if (containerSlot.second != 0) {
                containerSlot.first = 0;
                containerSlot.second = 0;
                sendInventoryMessage(*user, serial, slot);
            }
            break;
        }

        case CL_SWAP_ITEMS:
        {
            size_t obj1, slot1, obj2, slot2;
            iss >> obj1 >> del
                >> slot1 >> del
                >> obj2 >> del
                >> slot2 >> del;
            if (del != ']')
                return;
            Item::vect_t
                *containerFrom,
                *containerTo;
            if (obj1 == 0)
                containerFrom = &user->inventory();
            else {
                auto it = _objects.find(obj1);
                if (!isValidObject(client, *user, it))
                    break;
                containerFrom = &(const_cast<Object&>(*it).container());
            }
            if (obj2 == 0)
                containerTo = &user->inventory();
            else {
                auto it = _objects.find(obj2);
                if (!isValidObject(client, *user, it))
                    break;
                containerTo = &(const_cast<Object&>(*it).container());
            }

            if (slot1 >= containerFrom->size() || slot2 >= containerTo->size()) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }

            // Perform the swap
            auto
                &slotFrom = (*containerFrom)[slot1],
                &slotTo = (*containerTo)[slot2];
            auto temp = slotTo;
            slotTo = slotFrom;
            slotFrom = temp;

            sendInventoryMessage(*user, obj1, slot1);
            sendInventoryMessage(*user, obj2, slot2);
            break;
        }

        case CL_GET_INVENTORY:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != ']')
                return;
            auto it = _objects.find(serial);
            if (!isValidObject(client, *user, it))
                break;
            size_t slots = it->container().size();
            for (size_t i = 0; i != slots; ++i)
                sendInventoryMessage(*user, serial, i);
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }
    }
}

void Server::broadcast(MessageCode msgCode, const std::string &args){
    for (const User &user : _users){
        sendMessage(user.socket(), msgCode, args);
    }
}

void Server::gatherObject(size_t serial, User &user){
    // Give item to user
    const std::set<Object>::iterator it = _objects.find(serial);
    Object &obj = const_cast<Object &>(*it);
    const Item *const toGive = obj.chooseGatherItem();
    size_t qtyToGive = obj.chooseGatherQuantity(toGive);
    const size_t remaining = user.giveItem(toGive, qtyToGive);
    if (remaining > 0) {
        sendMessage(user.socket(), SV_INVENTORY_FULL);
        qtyToGive -= remaining;
    }
    // Remove tree if empty
    obj.removeItem(toGive, qtyToGive);
    if (obj.contents().isEmpty()) {
        // Ensure no other users are targeting this object, as it will be removed.
        for (const User &otherUserConst : _users) {
            const User & otherUser = const_cast<User &>(otherUserConst);
            if (&otherUser != &user &&
                otherUser.actionTarget() &&
                otherUser.actionTarget()->serial() == serial) {

                user.actionTarget(0);
                sendMessage(otherUser.socket(), SV_DOESNT_EXIST);
            }
        }
        broadcast(SV_REMOVE_OBJECT, makeArgs(serial));
        getCollisionChunk(obj.location()).removeObject(serial);
        _objects.erase(it);
    }

#ifdef SINGLE_THREAD
    saveData(_objects);
#else
    std::thread(saveData, _objects).detach();
#endif
}

bool Server::readUserData(User &user){
    XmlReader xr((std::string("Users/") + user.name() + ".usr").c_str());
    if (!xr)
        return false;

    auto elem = xr.findChild("location");
    Point p;
    if (!elem || !xr.findAttr(elem, "x", p.x) || !xr.findAttr(elem, "y", p.y)) {
            _debug("Invalid user data (location)", Color::RED);
            return false;
    }
    bool randomizedLocation = false;
    while (!isLocationValid(p, User::OBJECT_TYPE)) {
        p = mapRand();
        randomizedLocation = true;
    }
    if (randomizedLocation)
        _debug << Color::YELLOW << "Player " << user.name()
               << " was moved due to an invalid location." << Log::endl;
    user.location(p);

    for (auto elem : xr.getChildren("inventory")) {
        for (auto slotElem : xr.getChildren("slot", elem)) {
            int slot; std::string id; int qty;
            if (!xr.findAttr(slotElem, "slot", slot)) continue;
            if (!xr.findAttr(slotElem, "id", id)) continue;
            if (!xr.findAttr(slotElem, "quantity", qty)) continue;

            std::set<Item>::const_iterator it = _items.find(id);
            if (it == _items.end()) {
                _debug("Invalid user data (inventory item).  Removing item.", Color::RED);
                continue;
            }
            user.inventory(slot) =
                std::make_pair<const Item *, size_t>(&*it, static_cast<size_t>(qty));
        }
    }
    return true;

}

void Server::writeUserData(const User &user) const{
    XmlWriter xw(std::string("Users/") + user.name() + ".usr");

    auto e = xw.addChild("location");
    xw.setAttr(e, "x", user.location().x);
    xw.setAttr(e, "y", user.location().y);

    e = xw.addChild("inventory");
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const std::pair<const Item *, size_t> &slot = user.inventory(i);
        if (slot.first) {
            auto slotElement = xw.addChild("slot", e);
            xw.setAttr(slotElement, "slot", i);
            xw.setAttr(slotElement, "id", slot.first->id());
            xw.setAttr(slotElement, "quantity", slot.second);
        }
    }

    xw.publish();
}


void Server::sendMessage(const Socket &dstSocket, MessageCode msgCode,
                         const std::string &args) const{
    // Compile message
    std::ostringstream oss;
    oss << '[' << msgCode;
    if (args != "")
        oss << ',' << args;
    oss << ']';

    // Send message
    _socket.sendMessage(oss.str(), dstSocket);
}

void Server::loadData(){
    // Object types
    XmlReader xr("Data/objectTypes.xml");
    for (auto elem : xr.getChildren("objectType")) {
        std::string id;
        if (!xr.findAttr(elem, "id", id))
            continue;
        ObjectType ot(id);

        std::string s; int n;
        if (xr.findAttr(elem, "actionTime", n)) ot.actionTime(n);
        if (xr.findAttr(elem, "constructionTime", n)) ot.constructionTime(n);
        if (xr.findAttr(elem, "gatherReq", s)) ot.gatherReq(s);
        for (auto yield : xr.getChildren("yield", elem)) {
            if (!xr.findAttr(yield, "id", s))
                continue;
            double initMean = 1., initSD = 0, gatherMean = 1, gatherSD = 0;
            xr.findAttr(yield, "initialMean", initMean);
            xr.findAttr(yield, "initialSD", initSD);
            xr.findAttr(yield, "gatherMean", gatherMean);
            xr.findAttr(yield, "gatherSD", gatherSD);
            std::set<Item>::const_iterator itemIt = _items.insert(Item(s)).first;
            ot.addYield(&*itemIt, initMean, initSD, gatherMean, gatherSD);
        }
        auto collisionRect = xr.findChild("collisionRect", elem);
        if (collisionRect) {
            Rect r;
            xr.findAttr(collisionRect, "x", r.x);
            xr.findAttr(collisionRect, "y", r.y);
            xr.findAttr(collisionRect, "w", r.w);
            xr.findAttr(collisionRect, "h", r.h);
            ot.collisionRect(r);
        }
        for (auto objClass :xr.getChildren("class", elem))
            if (xr.findAttr(objClass, "name", s))
                ot.addClass(s);
        auto container = xr.findChild("container", elem);
        if (container) {
            if (xr.findAttr(container, "slots", n)) ot.containerSlots(n);
        }
        
        _objectTypes.insert(ot);
    }

    // Items
    xr.newFile("Data/items.xml");
    for (auto elem : xr.getChildren("item")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
            continue; // ID and name are mandatory.
        Item item(id);

        std::string s; int n;
        if (xr.findAttr(elem, "stackSize", n)) item.stackSize(n);
        if (xr.findAttr(elem, "constructs", s))
            // Create dummy ObjectType if necessary
            item.constructsObject(&*(_objectTypes.insert(ObjectType(s)).first));

        for (auto child : xr.getChildren("class", elem))
            if (xr.findAttr(child, "name", s)) item.addClass(s);
        
        std::pair<std::set<Item>::iterator, bool> ret = _items.insert(item);
        if (!ret.second) {
            Item &itemInPlace = const_cast<Item &>(*ret.first);
            itemInPlace = item;
        }
    }

    // Recipes
    xr.newFile("Data/recipes.xml");
    for (auto elem : xr.getChildren("recipe")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id))
            continue; // ID is mandatory.
        Recipe recipe(id);

        std::string s; int n;
        if (!xr.findAttr(elem, "product", s))
            continue; // product is mandatory.
        auto it = _items.find(s);
        if (it == _items.end()) {
            _debug << Color::RED << "Skipping recipe with invalid product " << s << Log::endl;
            continue;
        }
        recipe.product(&*it);

        if (xr.findAttr(elem, "time", n)) recipe.time(n);

        for (auto child : xr.getChildren("material", elem)) {
            int matQty = 1;
            xr.findAttr(child, "quantity", matQty);
            if (xr.findAttr(child, "id", s)) {
                auto it = _items.find(Item(s));
                if (it == _items.end()) {
                    _debug << Color::RED << "Skipping invalid recipe material " << s << Log::endl;
                    continue;
                }
                recipe.addMaterial(&*it, matQty);
            }
        }

        for (auto child : xr.getChildren("tool", elem)) {
            if (xr.findAttr(child, "name", s)) {
                recipe.addTool(s);
            }
        }
        
        _recipes.insert(recipe);
    }

    std::ifstream fs;
    // Detect/load state
    do {
        if (cmdLineArgs.contains("new"))
            break;

        // Map
        xr.newFile("World/map.world");
        if (!xr)
            break;
        auto elem = xr.findChild("size");
        if (!elem || !xr.findAttr(elem, "x", _mapX) || !xr.findAttr(elem, "y", _mapY)) {
            _debug("Map size missing or incomplete.", Color::RED);
            break;
        }
        _map = std::vector<std::vector<size_t> >(_mapX);
        for (size_t x = 0; x != _mapX; ++x)
            _map[x] = std::vector<size_t>(_mapY, 0);
        for (auto row : xr.getChildren("row")) {
            size_t y;
            if (!xr.findAttr(row, "y", y) || y >= _mapY)
                break;
            for (auto tile : xr.getChildren("tile", row)) {
                size_t x;
                if (!xr.findAttr(tile, "x", x) || x >= _mapX)
                    break;
                if (!xr.findAttr(tile, "terrain", _map[x][y]))
                    break;
            }
        }

        // Objects
        xr.newFile("World/objects.world");
        if (!xr)
            break;
        for (auto elem : xr.getChildren("object")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) {
                _debug("Skipping importing object with no type.", Color::RED);
                continue;
            }

            Point p;
            auto loc = xr.findChild("location", elem);
            if (!xr.findAttr(loc, "x", p.x) || !xr.findAttr(loc, "y", p.y)) {
                _debug("Skipping importing object with invalid/no location", Color::RED);
                continue;
            }

            std::set<ObjectType>::const_iterator it = _objectTypes.find(s);
            if (it == _objectTypes.end()) {
                _debug << Color::RED << "Skipping importing object with unknown type \"" << s
                       << "\"." << Log::endl;
                continue;
            }

            Object obj(&*it, p);

            size_t n;
            if (xr.findAttr(elem, "owner", s)) obj.owner(s);

            ItemSet contents;
            for (auto content : xr.getChildren("gatherable", elem)) {
                if (!xr.findAttr(content, "id", s))
                    continue;
                n = 1;
                xr.findAttr(content, "quantity", n);
                contents.set(&*_items.find(s), n);
            }
            obj.contents(contents);

            size_t q;
            for (auto inventory : xr.getChildren("inventory", elem)) {
                if (!xr.findAttr(inventory, "id", s))
                    continue;
                if (!xr.findAttr(inventory, "slot", n))
                    continue;
                q = 1;
                xr.findAttr(inventory, "qty", q);
                if (obj.container().size() <= n) {
                    _debug << Color::RED << "Skipping object with invalid inventory slot." << Log::endl;
                    continue;
                }
                auto &invSlot = obj.container()[n];
                invSlot.first = &*_items.find(s);
                invSlot.second = q;
            }

            _objects.insert(obj);
        }

        return;
    } while (false);

    _debug("No/invalid world data detected; generating new world.", Color::YELLOW);
    generateWorld();
}

void Server::saveData(const std::set<Object> &objects){
    // Map
#ifndef SINGLE_THREAD
    static std::mutex mapFileMutex;
    mapFileMutex.lock();
#endif
    XmlWriter xw("World/map.world");
    auto e = xw.addChild("size");
    const Server &server = Server::instance();
    xw.setAttr(e, "x", server._mapX);
    xw.setAttr(e, "y", server._mapY);
    for (size_t y = 0; y != server._mapY; ++y){
        auto row = xw.addChild("row");
        xw.setAttr(row, "y", y);
        for (size_t x = 0; x != server._mapX; ++x){
            auto tile = xw.addChild("tile", row);
            xw.setAttr(tile, "x", x);
            xw.setAttr(tile, "terrain", server._map[x][y]);
        }
    }
    xw.publish();
#ifndef SINGLE_THREAD
    mapFileMutex.unlock();
#endif

    // Objects
#ifndef SINGLE_THREAD
    static std::mutex objectsFileMutex;
    objectsFileMutex.lock();
#endif
    xw.newFile("World/objects.world");
    for (const Object &obj : objects) {
        if (!obj.type())
            continue;
        auto e = xw.addChild("object");

        xw.setAttr(e, "id", obj.type()->id());

        for (auto &content : obj.contents()) {
            auto contentE = xw.addChild("gatherable", e);
            xw.setAttr(contentE, "id", content.first->id());
            xw.setAttr(contentE, "quantity", content.second);
        }

        if (!obj.owner().empty())
            xw.setAttr(e, "owner", obj.owner());

        auto loc = xw.addChild("location", e);
        xw.setAttr(loc, "x", obj.location().x);
        xw.setAttr(loc, "y", obj.location().y);

        const auto container = obj.container();
        for (size_t i = 0; i != container.size(); ++i) {
            if (container[i].second == 0)
                continue;
            auto invSlotE = xw.addChild("inventory", e);
            xw.setAttr(invSlotE, "slot", i);
            xw.setAttr(invSlotE, "item", container[i].first->id());
            xw.setAttr(invSlotE, "qty", container[i].second);
        }
    }
    xw.publish();
#ifndef SINGLE_THREAD
    objectsFileMutex.unlock();
#endif
}

size_t Server::findStoneLayer(const Point &p, const std::vector<std::vector<size_t> > &stoneLayers) const{
    size_t y = static_cast<size_t>(p.y / TILE_H);
    if (y >= _mapY) {
        _debug << Color::RED << "Invalid location; clipping y from " << y << " to " << _mapY-1
               << ". original co-ord=" << p.y << Log::endl;
        y = _mapY-1;
    }
    double rawX = p.x;
    if (y % 2 == 1)
        rawX -= TILE_W/2;
    size_t x = static_cast<size_t>(rawX / TILE_W);
    if (x >= _mapX) {
        _debug << Color::RED << "Invalid location; clipping x from " << x << " to " << _mapX-1
               << ". original co-ord=" << p.x << Log::endl;
        x = _mapX-1;
    }
    return stoneLayers[x][y];
}

void Server::generateWorld(){
    _mapX = 60;
    _mapY = 60;

    // Grass by default
    _map = std::vector<std::vector<size_t> >(_mapX);
    for (size_t x = 0; x != _mapX; ++x){
        _map[x] = std::vector<size_t>(_mapY);
        for (size_t y = 0; y != _mapY; ++y)
            _map[x][y] = 0;
    }

    // River: randomish line
    const Point
        start(rand() % _mapX, rand() % _mapY),
        end(rand() % _mapX, rand() % _mapY);
    for (size_t x = 0; x != _mapX; ++x) {
        if (x % 3 == 0)
        _debug << "Generating river... " << 100 * x / _mapX << "%\r";
        for (size_t y = 0; y != _mapY; ++y) {
            Point thisTile(x, y);
            if (y % 2 == 1)
                thisTile.x -= .5;
            double dist = distance(thisTile, start, end) + randDouble() * 2 - 1;
            if (dist <=4)
                _map[x][y] = 1;
        }
    }
    _debug << "Generating river... 100%" << Log::endl;

    // Add tiles to chunks
    for (size_t x = 0; x != _mapX; ++x) {
        if (x % 2 == 0)
            _debug << "Dividing terrain into chunks... " << 100 * x / _mapX << "%\r";
        for (size_t y = 0; y != _mapY; ++y) {
            Point tileMidpoint(x * TILE_W, y * TILE_H + TILE_H / 2);
            if (y % 2 == 1)
                tileMidpoint.x += TILE_W / 2;
            // Add terrain info to adjacent chunks too
            auto superChunk = getCollisionSuperChunk(tileMidpoint);
            for (CollisionChunk *chunk : superChunk){
                size_t tile = _map[x][y];
                bool passable = tile != 1;
                chunk->addTile(x, y, passable);
            }
        }
    }
    _debug << "Dividing terrain into chunks... 100%" << Log::endl;

    //Generate stone layer
    size_t progress = 0;
    auto stoneLayer = _map;
    static const size_t LAYER_SIZE = 30;
    const size_t totalRegions = toInt((_mapY + .5) / LAYER_SIZE) * toInt((_mapX + .5) / LAYER_SIZE);
    for (size_t y = 0; y < _mapY; y += LAYER_SIZE)
        for (size_t x = 0; x < _mapX; x += LAYER_SIZE) {
            _debug << "Initializing stone regions... " << (100 * ++progress / totalRegions) << "%\r";
            size_t stoneType = rand() % 21;
            for (size_t xx = x; xx != x + LAYER_SIZE && xx != _mapX; ++xx)
                for (size_t yy = y; yy != y + LAYER_SIZE && yy != _mapX; ++yy)
                    stoneLayer[xx][yy] = stoneType;
        }
    _debug << "Initializing stone regions... 100%" << Log::endl;

    // Clay patches
    static const int numClayPatches = 7;
    const ObjectType *clay = &*_objectTypes.find(std::string("clay"));
    progress = 0;
    for (size_t i = 0; i != numClayPatches; ++i) {
        _debug << "Generating clay patches... " << 100 * ++progress / numClayPatches << "%\r";
        const size_t centerX = rand() % _mapX;
        const size_t centerY = rand() % _mapY;
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                Point tileCenter(centerX, centerY);
                const double dist = distance(tileCenter, thisTile);
                if (dist <= 6) {
                    if (isLocationValid(tileCenter, *clay))
                        addObject(clay, Point(thisTile.x * TILE_W, thisTile.y * TILE_H));
                }
            }
    }
    _debug << "Generating clay patches... 100%" << Log::endl;

    // Stone in circles
    static const size_t numStoneCircles = 10;
    std::vector<const ObjectType *> stoneObj[21];
    stoneObj[ 0].push_back(&*_objectTypes.find(std::string("Andesite")));
    pushBackMultiple(stoneObj[ 0], &*_objectTypes.find(std::string("copperAndesiteNormal")), 5);
    pushBackMultiple(stoneObj[ 0], &*_objectTypes.find(std::string("copperAndesitePoor")), 3);
    pushBackMultiple(stoneObj[ 0], &*_objectTypes.find(std::string("copperAndesiteRich")), 2);
    stoneObj[ 1].push_back(&*_objectTypes.find(std::string("Basalt")));
    pushBackMultiple(stoneObj[ 1], &*_objectTypes.find(std::string("copperBasaltNormal")), 5);
    pushBackMultiple(stoneObj[ 1], &*_objectTypes.find(std::string("copperBasaltPoor")), 3);
    pushBackMultiple(stoneObj[ 1], &*_objectTypes.find(std::string("copperBasaltRich")), 2);
    stoneObj[ 2].push_back(&*_objectTypes.find(std::string("Chalk")));
    stoneObj[ 3].push_back(&*_objectTypes.find(std::string("Chert")));
    stoneObj[ 4].push_back(&*_objectTypes.find(std::string("Claystone")));
    stoneObj[ 5].push_back(&*_objectTypes.find(std::string("Conglomerate")));
    stoneObj[ 6].push_back(&*_objectTypes.find(std::string("Dacite")));
    pushBackMultiple(stoneObj[ 6], &*_objectTypes.find(std::string("copperDaciteNormal")), 5);
    pushBackMultiple(stoneObj[ 6], &*_objectTypes.find(std::string("copperDacitePoor")), 3);
    pushBackMultiple(stoneObj[ 6], &*_objectTypes.find(std::string("copperDaciteRich")), 2);
    stoneObj[ 7].push_back(&*_objectTypes.find(std::string("Diorite")));
    stoneObj[ 8].push_back(&*_objectTypes.find(std::string("Dolomite")));
    stoneObj[ 9].push_back(&*_objectTypes.find(std::string("Gabbro")));
    stoneObj[10].push_back(&*_objectTypes.find(std::string("Gneiss")));
    pushBackMultiple(stoneObj[10], &*_objectTypes.find(std::string("copperGneissNormal")), 5);
    pushBackMultiple(stoneObj[10], &*_objectTypes.find(std::string("copperGneissPoor")), 3);
    pushBackMultiple(stoneObj[10], &*_objectTypes.find(std::string("copperGneissRich")), 2);
    stoneObj[11].push_back(&*_objectTypes.find(std::string("Granite")));
    stoneObj[12].push_back(&*_objectTypes.find(std::string("Limestone")));
    pushBackMultiple(stoneObj[12], &*_objectTypes.find(std::string("copperLimestoneNormal")), 5);
    pushBackMultiple(stoneObj[12], &*_objectTypes.find(std::string("copperLimestonePoor")), 3);
    pushBackMultiple(stoneObj[12], &*_objectTypes.find(std::string("copperLimestoneRich")), 2);
    stoneObj[13].push_back(&*_objectTypes.find(std::string("Marble")));
    pushBackMultiple(stoneObj[13], &*_objectTypes.find(std::string("copperMarbleNormal")), 5);
    pushBackMultiple(stoneObj[13], &*_objectTypes.find(std::string("copperMarblePoor")), 3);
    pushBackMultiple(stoneObj[13], &*_objectTypes.find(std::string("copperMarbleRich")), 2);
    pushBackMultiple(stoneObj[13], &*_objectTypes.find(std::string("copperMarble2Normal")), 5);
    pushBackMultiple(stoneObj[13], &*_objectTypes.find(std::string("copperMarble2Poor")), 3);
    pushBackMultiple(stoneObj[13], &*_objectTypes.find(std::string("copperMarble2Rich")), 2);
    stoneObj[14].push_back(&*_objectTypes.find(std::string("Phyllite")));
    pushBackMultiple(stoneObj[14], &*_objectTypes.find(std::string("copperPhylliteNormal")), 5);
    pushBackMultiple(stoneObj[14], &*_objectTypes.find(std::string("copperPhyllitePoor")), 3);
    pushBackMultiple(stoneObj[14], &*_objectTypes.find(std::string("copperPhylliteRich")), 2);
    stoneObj[15].push_back(&*_objectTypes.find(std::string("Quartzite")));
    pushBackMultiple(stoneObj[15], &*_objectTypes.find(std::string("copperQuartziteNormal")), 5);
    pushBackMultiple(stoneObj[15], &*_objectTypes.find(std::string("copperQuartzitePoor")), 3);
    pushBackMultiple(stoneObj[15], &*_objectTypes.find(std::string("copperQuartziteRich")), 2);
    stoneObj[16].push_back(&*_objectTypes.find(std::string("Rhyolite")));
    pushBackMultiple(stoneObj[16], &*_objectTypes.find(std::string("copperRhyoliteNormal")), 5);
    pushBackMultiple(stoneObj[16], &*_objectTypes.find(std::string("copperRhyolitePoor")), 3);
    pushBackMultiple(stoneObj[16], &*_objectTypes.find(std::string("copperRhyoliteRich")), 2);
    stoneObj[17].push_back(&*_objectTypes.find(std::string("RockSalt")));
    stoneObj[18].push_back(&*_objectTypes.find(std::string("Schist")));
    pushBackMultiple(stoneObj[18], &*_objectTypes.find(std::string("copperSchistNormal")), 5);
    pushBackMultiple(stoneObj[18], &*_objectTypes.find(std::string("copperSchistPoor")), 3);
    pushBackMultiple(stoneObj[18], &*_objectTypes.find(std::string("copperSchistRich")), 2);
    stoneObj[19].push_back(&*_objectTypes.find(std::string("Shale")));
    stoneObj[20].push_back(&*_objectTypes.find(std::string("Slate")));
    pushBackMultiple(stoneObj[20], &*_objectTypes.find(std::string("copperSlateNormal")), 5);
    pushBackMultiple(stoneObj[20], &*_objectTypes.find(std::string("copperSlatePoor")), 3);
    pushBackMultiple(stoneObj[20], &*_objectTypes.find(std::string("copperSlateRich")), 2);
    progress = 0;
    for (int i = 0; i != numStoneCircles; ++i) {
        _debug << "Generating stone deposits... " << (100 * ++progress / numStoneCircles) << "%\r";
        const size_t centerX = rand() % _mapX;
        const size_t centerY = rand() % _mapY;
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                Point tileCenter(centerX, centerY);
                const double dist = distance(tileCenter, thisTile);
                if (dist <= 6) {
                    size_t stoneIndex = 0;
                    const std::vector<const ObjectType *> stoneVec = stoneObj[stoneLayer[x][y]];
                    if (rand() % 10 == 0 && stoneVec.size() > 1) {
                        // This object will be a random ore/mineral
                        stoneIndex = rand() % (stoneVec.size() - 1) + 1;
                    }
                    const ObjectType *type = stoneVec[stoneIndex];
                    assert(type);
                    if (isLocationValid(tileCenter, *type))
                        addObject(type, Point(thisTile.x * TILE_W, thisTile.y * TILE_H));
                }
            }
    }
    _debug << "Generating stone deposits... 100%" << Log::endl;

    // Rocks/grass/seaweed
    const ObjectType *rock[21];
    rock[ 0] = &*_objectTypes.find(std::string("AndesiteRock"));
    rock[ 1] = &*_objectTypes.find(std::string("BasaltRock"));
    rock[ 2] = &*_objectTypes.find(std::string("ChalkRock"));
    rock[ 3] = &*_objectTypes.find(std::string("ChertRock"));
    rock[ 4] = &*_objectTypes.find(std::string("ClaystoneRock"));
    rock[ 5] = &*_objectTypes.find(std::string("ConglomerateRock"));
    rock[ 6] = &*_objectTypes.find(std::string("DaciteRock"));
    rock[ 7] = &*_objectTypes.find(std::string("DioriteRock"));
    rock[ 8] = &*_objectTypes.find(std::string("DolomiteRock"));
    rock[ 9] = &*_objectTypes.find(std::string("GabbroRock"));
    rock[10] = &*_objectTypes.find(std::string("GneissRock"));
    rock[11] = &*_objectTypes.find(std::string("GraniteRock"));
    rock[12] = &*_objectTypes.find(std::string("LimestoneRock"));
    rock[13] = &*_objectTypes.find(std::string("MarbleRock"));
    rock[14] = &*_objectTypes.find(std::string("PhylliteRock"));
    rock[15] = &*_objectTypes.find(std::string("QuartziteRock"));
    rock[16] = &*_objectTypes.find(std::string("RhyoliteRock"));
    rock[17] = &*_objectTypes.find(std::string("RockSaltRock"));
    rock[18] = &*_objectTypes.find(std::string("SchistRock"));
    rock[19] = &*_objectTypes.find(std::string("ShaleRock"));
    rock[20] = &*_objectTypes.find(std::string("SlateRock"));
    
    const ObjectType *stick = &*_objectTypes.find(std::string("stick"));
    const ObjectType *seaweed = &*_objectTypes.find(std::string("seaweed"));
    const ObjectType *grass = &*_objectTypes.find(std::string("grass"));
    
    static const size_t numObjects = 300;
    progress = 0;
    for (int i = 0; i != numObjects; ++i){
        if (i % 25 == 0)
            _debug << "Generating objects... " << (100 * i / (numObjects-1)) << "%" << "\r";
        Point p;
        const ObjectType *type = 0;
        do {
            p = mapRand();
            size_t terrain = findTile(p);
            if (terrain == 1)
                type = seaweed;
            else {
                size_t randNum = rand() % 8;
                if (randNum < 4)
                    type = grass;
                else if (randNum < 6)
                    type = stick;
                else
                    type = rock[findStoneLayer(p, stoneLayer)];
            }
        } while (!isLocationValid(p, *type));
        addObject(type, p);
    }
    _debug << "Generating objects... 100%" << Log::endl;

    // Trees
    const ObjectType *ashTree = &*_objectTypes.find(std::string("ashTree"));
    for (int i = 0; i != 5; ++i){
        Point p;
        do {
            p = mapRand();
        } while (!isLocationValid(p, *ashTree));
        addObject(ashTree, p);
    }
}

Point Server::mapRand() const{
    return Point(randDouble() * (_mapX - 0.5) * TILE_W,
                 randDouble() * _mapY * TILE_H);
}

bool Server::itemIsClass(const Item *item, const std::string &className) const{
    assert (item);
    return item->isClass(className);
}

void Server::addObject (const ObjectType *type, const Point &location, const User *owner){
    Object newObj(type, location);
    if (owner)
        newObj.owner(owner->name());
    auto it = _objects.insert(newObj).first;
    broadcast(SV_OBJECT, makeArgs(newObj.serial(), location.x, location.y, type->id()));
    if (owner)
        broadcast(SV_OWNER, makeArgs(newObj.serial(), newObj.owner()));

    // Add item to relevant chunk
    if (type->collides())
        getCollisionChunk(location).addObject(&*it);
}

void Server::sendInventoryMessage(const User &user, size_t serial, size_t slot) const{
    const Item::vect_t *container;
    if (serial == 0)
        container = &user.inventory();
    else {
        auto it = _objects.find(serial);
        if (it == _objects.end())
            return; // Object doesn't exist.
        else
            container = &it->container();
    }
    if (slot >= container->size()) {
        sendMessage(user.socket(), SV_INVALID_SLOT);
        return;
    }
    auto containerSlot = (*container)[slot];
    std::string itemID = containerSlot.first ? containerSlot.first->id() : "none";
    sendMessage(user.socket(), SV_INVENTORY, makeArgs(serial, slot, itemID, containerSlot.second));
}
