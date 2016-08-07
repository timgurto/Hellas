// (C) 2015-2016 Tim Gurto

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
#include "../messageCodes.h"
#include "../util.h"

extern Args cmdLineArgs;

Server *Server::_instance = nullptr;
LogConsole *Server::_debugInstance = nullptr;

const int Server::MAX_CLIENTS = 20;

const ms_t Server::CLIENT_TIMEOUT = 10000;
const ms_t Server::MAX_TIME_BETWEEN_LOCATION_UPDATES = 300;

const ms_t Server::SAVE_FREQUENCY = 1000;

const double Server::MOVEMENT_SPEED = 80;
const px_t Server::ACTION_DISTANCE = 30;
const px_t Server::CULL_DISTANCE = 100;
const px_t Server::TILE_W = 32;
const px_t Server::TILE_H = 32;

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

    // Send welcome message
    sendMessage(socket, SV_WELCOME);

    // Send him his health
    sendMessage(newUser.socket(), SV_HEALTH, makeArgs(newUser.health()));

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
    for (const User *userP : findUsersInArea(newUser.location()))
        sendMessage(newUser.socket(), SV_LOCATION, userP->makeLocationCommand());

    // Send him object details
    const Point &loc = newUser.location();
    auto loX = _objectsByX.lower_bound(&Object(Point(loc.x - CULL_DISTANCE, 0)));
    auto hiX = _objectsByX.upper_bound(&Object(Point(loc.x + CULL_DISTANCE, 0)));
    for (auto it = loX; it != hiX; ++it){
        const Object &obj = **it;
        assert (obj.type() != nullptr);
        if (abs(obj.location().y - loc.y) > CULL_DISTANCE) // Cull y
            continue;
        sendMessage(newUser.socket(), SV_OBJECT,
                    makeArgs(obj.serial(), obj.location().x, obj.location().y, obj.type()->id()));
        if (!obj.owner().empty())
            sendMessage(newUser.socket(), SV_OWNER, makeArgs(obj.serial(), obj.owner()));
    }

    // Send him his inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        if (newUser.inventory(i).first != nullptr)
            sendInventoryMessage(newUser, i);
    }

    // Add new user to list, and broadcast his location
    std::set<User>::const_iterator it = _users.insert(newUser).first;
    _usersByName[name] = &*it;
    broadcast(SV_LOCATION, newUser.makeLocationCommand());

    // Add user to location-indexed trees
    _usersByX.insert(&*it);
    _usersByY.insert(&*it);
}

void Server::removeUser(const std::set<User>::iterator &it){
        // Broadcast message
        broadcast(SV_USER_DISCONNECTED, it->name());

        // Save user data
        writeUserData(*it);

        _usersByX.erase(&*it);
        _usersByY.erase(&*it);
        _usersByName.erase(it->name());

        _users.erase(it);
}

void Server::removeUser(const Socket &socket){
    const std::set<User>::iterator it = _users.find(socket);
    if (it != _users.end())
        removeUser(it);
}

std::list<const User *> Server::findUsersInArea(Point loc, double squareRadius) const{
    std::list<const User *> users;
    auto loX = _usersByX.lower_bound(&User(Point(loc.x - squareRadius, 0)));
    auto hiX = _usersByX.upper_bound(&User(Point(loc.x + squareRadius, 0)));
    for (auto it = loX; it != hiX; ++it)
        if (abs(loc.y - (*it)->location().y) <= squareRadius)
            users.push_back(*it);

    return users;
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

void Server::removeObject(Object &obj, const User *userToExclude){
    // Ensure no other users are targeting this object, as it will be removed.
    size_t serial = obj.serial();
    for (const User &userConst : _users) {
        User & user = const_cast<User &>(userConst);
        if (&user != userToExclude &&
            user.action() == User::GATHER &&
            user.actionObject()->serial() == serial) {

            user.action(User::NO_ACTION);
            sendMessage(user.socket(), SV_DOESNT_EXIST);
        }
    }

    // Alert nearby users of the removal
    for (const User *userP : findUsersInArea(obj.location()))
        sendMessage(userP->socket(), SV_REMOVE_OBJECT, makeArgs(serial));

    getCollisionChunk(obj.location()).removeObject(serial);
    _objectsByX.erase(&obj);
    _objectsByY.erase(&obj);
    _objects.erase(obj);

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
    // Remove object if empty
    obj.removeItem(toGive, qtyToGive);
    if (obj.contents().isEmpty()) {
        removeObject(obj, &user);
    }

#ifdef SINGLE_THREAD
    saveData(_objects);
#else
    std::thread(saveData, _objects).detach();
#endif
}

void Server::generateWorld(){
    _mapX = 30;
    _mapY = 30;

    static const char
        GRASS = 0,
        STONE = 1,
        ROAD = 2,
        WATER = 4,
        DEEP_WATER = 3;

    // Grass by default
    _map = std::vector<std::vector<size_t> >(_mapX);
    for (size_t x = 0; x != _mapX; ++x){
        _map[x] = std::vector<size_t>(_mapY);
        for (size_t y = 0; y != _mapY; ++y)
            _map[x][y] = GRASS;
    }

    // Stone in circles
    for (int i = 0; i != 5; ++i) {
        const size_t centerX = rand() % _mapX;
        const size_t centerY = rand() % _mapY;
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                const double dist = distance(Point(centerX, centerY), thisTile);
                if (dist <= 4)
                    _map[x][y] = STONE;
            }
    }

    // Roads
    for (int i = 0; i != 2; ++i) {
        const Point
            start(rand() % _mapX, rand() % _mapY),
            end(rand() % _mapX, rand() % _mapY);
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                double dist = distance(thisTile, start, end);
                if (dist <= 1)
                    _map[x][y] = ROAD;
            }
    }

    // River: randomish line
    const Point
        start(rand() % _mapX, rand() % _mapY),
        end(rand() % _mapX, rand() % _mapY);
    for (size_t x = 0; x != _mapX; ++x)
        for (size_t y = 0; y != _mapY; ++y) {
            Point thisTile(x, y);
            if (y % 2 == 1)
                thisTile.x -= .5;
            double dist = distance(thisTile, start, end) + randDouble() * 2 - 1;
            if (dist <= 0.5)
                _map[x][y] = DEEP_WATER;
            else if (dist <= 2)
                _map[x][y] = WATER;
        }

    const ObjectType *const branch = &*_objectTypes.find(std::string("branch"));
    for (int i = 0; i != 30; ++i){
        Point loc;
        do {
            loc = mapRand();
        } while (!isLocationValid(loc, *branch));
        addObject(branch, loc);
    }

    const ObjectType *const tree = &*_objectTypes.find(std::string("tree"));
    for (int i = 0; i != 10; ++i) {
        Point loc;
        do {
            loc = mapRand();
        } while (!isLocationValid(loc, *tree));
        addObject(tree, loc);
    }

    const ObjectType *const chest = &*_objectTypes.find(std::string("chest"));
    for (int i = 0; i != 10; ++i) {
        Point loc;
        do {
            loc = mapRand();
        } while (!isLocationValid(loc, *chest));
        addObject(chest, loc);
    }

    const ObjectType *const tradeCart = &*_objectTypes.find(std::string("tradeCart"));
    for (int i = 0; i != 20; ++i) {
        Point loc;
        do {
            loc = mapRand();
        } while (!isLocationValid(loc, *tradeCart));
        addObject(tradeCart, loc);
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

Object &Server::addObject (const ObjectType *type, const Point &location, const User *owner){
    Object newObj(type, location);
    if (owner != nullptr)
        newObj.owner(owner->name());
    auto it = _objects.insert(newObj).first;

    // Alert nearby users
    for (const User *userP : findUsersInArea(location)){
        sendMessage(userP->socket(), SV_OBJECT,
                    makeArgs(newObj.serial(), location.x, location.y, type->id()));
        if (owner != nullptr)
            sendMessage(userP->socket(), SV_OWNER, makeArgs(newObj.serial(), newObj.owner()));
    }

    // Add object to relevant chunk
    if (type->collides())
        getCollisionChunk(location).addObject(&*it);

    // Add object to x/y index sets
    _objectsByX.insert(&*it);
    _objectsByY.insert(&*it);

    return const_cast<Object&>(*it);
}

const User &Server::getUserByName(const std::string &username) const {
    return *_usersByName.find(username)->second;
}

