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
#include "NPC.h"
#include "NPCType.h"
#include "Object.h"
#include "ObjectType.h"
#include "Recipe.h"
#include "Server.h"
#include "User.h"
#include "Vehicle.h"
#include "VehicleType.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../util.h"

extern Args cmdLineArgs;

Server *Server::_instance = nullptr;
LogConsole *Server::_debugInstance = nullptr;

const int Server::MAX_CLIENTS = 20;

const ms_t Server::CLIENT_TIMEOUT = 10000;
const ms_t Server::MAX_TIME_BETWEEN_LOCATION_UPDATES = 300;

const ms_t Server::SAVE_FREQUENCY = 1000;

const px_t Server::ACTION_DISTANCE = 30;
const px_t Server::CULL_DISTANCE = 320;
const px_t Server::TILE_W = 32;
const px_t Server::TILE_H = 32;

Server::Server():
_time(SDL_GetTicks()),
_lastTime(_time),
_socket(),
_loop(false),
_running(false),
_mapX(0),
_mapY(0),
_debug("server.log"),
_lastSave(_time),
_dataLoaded(false){
    _instance = this;
    _debugInstance = &_debug;
    if (cmdLineArgs.contains("quiet"))
        _debug.quiet();

    _debug << cmdLineArgs << Log::endl;
    if (Socket::debug == nullptr)
        Socket::debug = &_debug;

    User::init();

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
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
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
                _debug << "Client " << raw << " disconnected; error code: " << err << Log::endl;
                removeUser(raw);
                closesocket(raw);
                _clientSockets.erase(it++);
                continue;
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
    if (!_dataLoaded)
        loadData();

    _loop = true;
    _running = true;
    while (_loop) {
        _time = SDL_GetTicks();
        const ms_t timeElapsed = _time - _lastTime;
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

        // Update objects
        for (Object *objP : _objects)
            objP->update(timeElapsed);

        // Clean up dead objects
        for (Object *objP : _objectsToRemove){
            removeObject(*objP);
        }
        _objectsToRemove.clear();

        // Update spawners
        for (auto &pair : _spawners)
            pair.second.update(_time);

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
    _running = false;
}

void Server::addUser(const Socket &socket, const std::string &name){
    User newUser(name, 0, socket);
    const bool userExisted = readUserData(newUser);
    if (!userExisted) {
        newUser.setClass(User::Class(rand() % User::NUM_CLASSES));
        static const Point SPAWN_LOC(39370, 20100);
        static const double SPAWN_RANGE = 50;
        Point newLoc;
        size_t attempts = 0;
        do {
            _debug << "Attempt #" << ++attempts << " at placing new user" << Log::endl;
            newLoc.x = (randDouble() * 2 - 1) * SPAWN_RANGE + SPAWN_LOC.x;
            newLoc.y = (randDouble() * 2 - 1) * SPAWN_RANGE + SPAWN_LOC.y;
        } while (!isLocationValid(newLoc, User::OBJECT_TYPE));
        newUser.location(newLoc);
        _debug << "New";
    } else {
        _debug << "Existing";
    }
    _debug << " user, " << name << " has logged in." << Log::endl;

    // Send welcome message
    sendMessage(socket, SV_WELCOME);

    // Send him his own location
    sendMessage(newUser.socket(), SV_LOCATION, newUser.makeLocationCommand());

    // Send him his class
    sendMessage(newUser.socket(), SV_CLASS, makeArgs(name, newUser.className()));

    // Send him his health
    sendMessage(newUser.socket(), SV_HEALTH, makeArgs(newUser.health()));

    for (const User *userP : findUsersInArea(newUser.location())){
        // Send him information about other nearby users
        sendUserInfo(newUser, *userP);
        // Send nearby others this user's information
        sendUserInfo(*userP, newUser);
    }

    // Send him object details
    const Point &loc = newUser.location();
    auto loX = _objectsByX.lower_bound(&Object(Point(loc.x - CULL_DISTANCE, 0)));
    auto hiX = _objectsByX.upper_bound(&Object(Point(loc.x + CULL_DISTANCE, 0)));
    for (auto it = loX; it != hiX; ++it){
        const Object &obj = **it;
        if (obj.type() == nullptr){
            _debug("Null-type object skipped", Color::RED);
            continue;
        }
        if (abs(obj.location().y - loc.y) > CULL_DISTANCE) // Cull y
            continue;
        sendObjectInfo(newUser, obj);
    }

    // For easier testing/debugging: grant full stacks of everything
    if (isDebug() && !userExisted)
        for (const ServerItem &item : _items)
            newUser.giveItem(&item, item.stackSize());

    // Send him his inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        if (newUser.inventory(i).first != nullptr)
            sendInventoryMessage(newUser, i, INVENTORY);
    }

    // Send him his gear
    for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
        if (newUser.gear(i).first != nullptr)
            sendInventoryMessage(newUser, i, GEAR);
    }

    // Calculate and send him his stats
    newUser.updateStats();

    // Add new user to list
    std::set<User>::const_iterator it = _users.insert(newUser).first;
    _usersByName[name] = &*it;

    // Add user to location-indexed trees
    const User *userP = &*it;
    _usersByX.insert(userP);
    _usersByY.insert(userP);
    _objectsByX.insert(userP);
    _objectsByY.insert(userP);
}

void Server::removeUser(const std::set<User>::iterator &it){
    // Alert nearby users
    for (const User *userP : findUsersInArea(it->location()))
        if (userP != &*it)
            sendMessage(userP->socket(), SV_USER_DISCONNECTED, it->name());

    forceUntarget(*it);

    // Save user data
    writeUserData(*it);

    _usersByX.erase(&*it);
    _usersByY.erase(&*it);
    _usersByName.erase(it->name());

    _users.erase(it);
}

void Server::removeUser(const Socket &socket){
    const std::set<User>::iterator it = _users.find(socket);
    assert (it != _users.end()); // FIXME this occasionally fails
    removeUser(it);
}

std::list<User *> Server::findUsersInArea(Point loc, double squareRadius) const{
    std::list<User *> users;
    auto loX = _usersByX.lower_bound(&User(Point(loc.x - squareRadius, 0)));
    auto hiX = _usersByX.upper_bound(&User(Point(loc.x + squareRadius, 0)));
    for (auto it = loX; it != hiX; ++it)
        if (abs(loc.y - (*it)->location().y) <= squareRadius)
            users.push_back(const_cast<User *>(*it));

    return users;
}

bool Server::isObjectInRange(const Socket &client, const User &user, const Object *obj) const{
    // Object doesn't exist
    if (obj == nullptr) {
        sendMessage(client, SV_DOESNT_EXIST);
        return false;
    }

    // Check distance from user
    if (distance(user.collisionRect(), obj->collisionRect()) > ACTION_DISTANCE) {
        sendMessage(client, SV_TOO_FAR);
        return false;
    }

    return true;
}

void Server::forceUntarget(const Object &target, const User *userToExclude){
    // Fix users targeting the object
    size_t serial = target.serial();
    for (const User &userConst : _users) {
        User & user = const_cast<User &>(userConst);
        if (&user == userToExclude)
            continue;
        if (target.classTag() == 'n'){
            const NPC &npc = dynamic_cast<const NPC &>(target);
            if (user.action() == User::ATTACK && user.target() == &npc) {
                user.finishAction();
                continue;
            }
        }
        if (user.action() == User::GATHER && user.actionObject()->serial() == serial) {
            sendMessage(user.socket(), SV_DOESNT_EXIST);
            user.cancelAction();
        }
    }

    // Fix NPCs targeting the object
    for (const Object *pObj : _objects) {
        Object &obj = *const_cast<Object *>(pObj);
        if (obj.classTag() == 'n'){
            NPC &npc = dynamic_cast<NPC &>(obj);
            if (npc.target() == &target)
                npc.target(nullptr);
        }
    }
}

void Server::removeObject(Object &obj, const User *userToExclude){
    obj.onRemove();

    // Ensure no other users are targeting this object, as it will be removed.
    forceUntarget(obj, userToExclude);

    // Alert nearby users of the removal
    size_t serial = obj.serial();
    for (const User *userP : findUsersInArea(obj.location()))
        sendMessage(userP->socket(), SV_REMOVE_OBJECT, makeArgs(serial));

    getCollisionChunk(obj.location()).removeObject(serial);
    _objectsByX.erase(&obj);
    _objectsByY.erase(&obj);
    _objects.erase(&obj);

}

void Server::gatherObject(size_t serial, User &user){
    // Give item to user
    Object *obj = findObject(serial);
    const ServerItem *const toGive = obj->chooseGatherItem();
    size_t qtyToGive = obj->chooseGatherQuantity(toGive);
    const size_t remaining = user.giveItem(toGive, qtyToGive);
    if (remaining > 0) {
        sendMessage(user.socket(), SV_INVENTORY_FULL);
        qtyToGive -= remaining;
    }
    // Remove object if empty
    obj->removeItem(toGive, qtyToGive);
    if (obj->contents().isEmpty()) {
        removeObject(*obj, &user);
    } else {
        obj->decrementGatheringUsers();
    }

#ifdef SINGLE_THREAD
    saveData(_objects);
#else
    std::thread(saveData, _objects).detach();
#endif
}

void Server::spawnInitialObjects(){

    static const char
        GRASS = 0,
        STONE = 1,
        ROAD = 2,
        WATER = 4,
        DEEP_WATER = 3,
        CLAY=5;

    std::vector<const Object *> trees;
    const ObjectType *const tree = findObjectTypeByName("tree");
    if (tree != nullptr)
        for (int i = 0; i != 10; ++i){
            Point loc;
            do {
                loc = mapRand();
            } while (!isLocationValid(loc, *tree));
            Object &obj = addObject(tree, loc);
            trees.push_back(&obj);
        }

    /*
    Random objects:
     - Grass, near tree: stick
     - Grass, otherwise: grass
     - Stone: rock/tin/copper
     - Clay: clay
    */
    size_t objects = 200;
    for (size_t i = 0; i != objects; ++i){
        Point loc(mapRand());
        auto coords = getTileCoords(loc);
        size_t terrain = _map[coords.first][coords.second];

        std::string typeName;
        switch(terrain){
        case GRASS:
            for (auto tree : trees){
                if (distance(loc, tree->location()) <= 100){
                    typeName = "stick";
                    break;
                }
            }
            if (typeName.empty())
                typeName = "grass";
            break;

        case STONE:
        {
            unsigned random = rand() % 20;
            if (random == 0)
                typeName = "tin";
            else if (random == 1)
                typeName = "copper";
            else
                typeName = "rock";
            break;
        }

        case CLAY:
            typeName = "clay";
            break;

        default:
            ++objects;
            continue;
        }

        const ObjectType &type = *findObjectTypeByName(typeName);
        if (!isLocationValid(loc, type)){
            ++objects;
            continue;
        }
        addObject(&type, loc);
    }

    // NPCs
    const ObjectType *const critterObj = findObjectTypeByName("critter");
    if (critterObj != nullptr){
        const NPCType *const critter = dynamic_cast<const NPCType *const>(critterObj);
        for (int i = 0; i != 20; ++i) {
            Point loc;
            do {
                loc = mapRand();
            } while (!isLocationValid(loc, *critter));
            addObject(new NPC(critter, loc));
        }
    }
    const ObjectType *const boarObj = findObjectTypeByName("boar");
    if (boarObj != nullptr){
        const NPCType *const boar = dynamic_cast<const NPCType *const>(boarObj);
        for (int i = 0; i != 5; ++i) {
            Point loc;
            do {
                loc = mapRand();
            } while (!isLocationValid(loc, *boar));
            addObject(new NPC(boar, loc));
        }
    }


    // From spawners
    for (auto &pair: _spawners){
        Spawner &spawner = pair.second;
        assert(spawner.type() != nullptr);
        for (size_t i = 0; i != spawner.quantity(); ++i)
            spawner.spawn();
    }
}

Point Server::mapRand() const{
    return Point(randDouble() * (_mapX - 0.5) * TILE_W,
                 randDouble() * _mapY * TILE_H);
}

bool Server::itemIsTag(const ServerItem *item, const std::string &tagName) const{
    assert (item);
    return item->isTag(tagName);
}

const ObjectType *Server::findObjectTypeByName(const std::string &id) const{
    for (const ObjectType *type : _objectTypes)
        if (type->id() == id)
            return type;
    return nullptr;
}

Object &Server::addObject(const ObjectType *type, const Point &location, const User *owner){
    Object *newObj = type->classTag() == 'v' ?
            new Vehicle(dynamic_cast<const VehicleType *>(type), location) :
            new Object(type, location);
    if (owner != nullptr)
        newObj->owner(owner->name());
    return addObject(newObj);
}

NPC &Server::addNPC(const NPCType *type, const Point &location){
    NPC *newNPC = new NPC(type, location);
    return dynamic_cast<NPC &>(addObject(newNPC));
}

Object &Server::addObject(Object *newObj){
    auto it = _objects.insert(newObj).first;
    const Point &loc = newObj->location();

    // Alert nearby users
    for (const User *userP : findUsersInArea(loc))
        sendObjectInfo(*userP, *newObj);

    // Add object to relevant chunk
    if (newObj->type()->collides())
        getCollisionChunk(loc).addObject(*it);

    // Add object to x/y index sets
    _objectsByX.insert(*it);
    _objectsByY.insert(*it);

    return const_cast<Object&>(**it);
}

const User &Server::getUserByName(const std::string &username) const {
    return *_usersByName.find(username)->second;
}

Object *Server::findObject(size_t serial){
    auto it = _objects.find(&Object(serial));
    if (it == _objects.end())
        return nullptr;
    else
        return *it;
}

Object *Server::findObject(const Point &loc){
    auto it = _objects.find(&Object(loc));
    if (it == _objects.end())
        return nullptr;
    else
        return *it;
}
