#include <SDL.h>
#include <sstream>
#include <fstream>
#include <utility>

#include "Client.h" //TODO remove; only here for random initial placement
#include "Renderer.h"
#include "Socket.h"
#include "Server.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

extern Args cmdLineArgs;
extern Renderer renderer;

const int Server::MAX_CLIENTS = 20;
const size_t Server::BUFFER_SIZE = 1023;

const Uint32 Server::CLIENT_TIMEOUT = 10000;

const Uint32 Server::SAVE_FREQUENCY = 1000;

const int Server::ACTION_DISTANCE = 20;

bool Server::isServer = false;

const int Server::TILE_W = 32;
const int Server::TILE_H = 32;

Server::Server():
_loop(true),
_debug(100),
_socket(),
_time(SDL_GetTicks()),
_lastTime(_time),
_lastSave(_time),
_mapX(0),
_mapY(0){
    isServer = true;

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
    _debug << "Server address: " << inet_ntoa(serverAddr.sin_addr) << ":" << ntohs(serverAddr.sin_port) << Log::endl;
    _socket.listen();

    _debug("Listening for connections");
}

Server::~Server(){
    saveData();
}

void Server::checkSockets(){
    // Populate socket list with active sockets
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    for (std::set<Socket>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it)
        FD_SET(it->getRaw(), &readFDs);

    // Poll for activity
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }
    _time = SDL_GetTicks();

    // Activity on server socket: new connection
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {

        int addrSize = sizeof(sockaddr);
        if (_clientSockets.size() == MAX_CLIENTS) {
            _debug("No room for additional clients; all slots full");
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            Socket s(tempSocket);
            s.delayClosing(5000); // Allow time for rejection message to be sent before closing socket
            sendMessage(s, SV_SERVER_FULL);
        } else {
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            if (tempSocket == SOCKET_ERROR) {
                _debug << Color::RED << "Error accepting connection: " << WSAGetLastError() << Log::endl;
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
            int charsRead = recv(raw, buffer, BUFFER_SIZE, 0);
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
                removeUser(it++);
            } else {
                ++it;
            }
        }

        // Save user data
        if (_time - _lastSave >= SAVE_FREQUENCY) {
            for (std::set<User>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
                writeUserData(*it);
            }
            saveData();
            _lastSave = _time;
        }

        // Update users
        for (std::set<User>::iterator it = _users.begin(); it != _users.end(); ++it)
            it->update(timeElapsed, *this);

        // Deal with any messages from the server
        while (!_messages.empty()){
            handleMessage(_messages.front().first, _messages.front().second);
            _messages.pop();
        }

        // Handle user events
        static SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            switch(e.type) {
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE){
                    _loop = false;
                }
                break;
            case SDL_QUIT:
                _loop = false;
                break;

            default:
                // Unhandled event
                ;
            }
        }

        draw();

        checkSockets();

        SDL_Delay(10);
    }

    // Save all user data
    for(std::set<User>::const_iterator it = _users.begin(); it != _users.end(); ++it){
        writeUserData(*it);
    }
}

void Server::draw() const{
    renderer.clear();
    _debug.draw();
    renderer.present();
}

void Server::addUser(const Socket &socket, const std::string &name){
    User newUser(name, 0, socket);
    bool userExisted = readUserData(newUser);
    if (!userExisted) {
        newUser.location(mapRand());
        newUser.inventory(0) = std::make_pair("axe", 1);
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
    for (std::set<User>::const_iterator it = _users.begin(); it != _users.end(); ++it)
        sendMessage(newUser.socket(), SV_LOCATION, it->makeLocationCommand());

    // Send him branch locations
    for (std::set<BranchLite>::const_iterator it = _branches.begin(); it != _branches.end(); ++it)
        sendMessage(newUser.socket(), SV_BRANCH, makeArgs(it->serial, it->location.x, it->location.y));

    // Send him tree locations
    for (std::set<TreeLite>::const_iterator it = _trees.begin(); it != _trees.end(); ++it)
        sendMessage(newUser.socket(), SV_TREE, makeArgs(it->serial, it->location.x, it->location.y));

    // Send him his inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        if (newUser.inventory(i).first != "none")
            sendMessage(socket, SV_INVENTORY, makeArgs(i,
                                                       newUser.inventory(i).first,
                                                       newUser.inventory(i).second));
    }

    // Add new user to list, and broadcast his location
    _users.insert(newUser);
    broadcast(SV_LOCATION, newUser.makeLocationCommand());
}

void Server::removeUser(std::set<User>::iterator &it){
        // Broadcast message
        broadcast(SV_USER_DISCONNECTED, it->name());

        // Save user data
        writeUserData(*it);

        _usernames.erase(it->name());
        _users.erase(it);
}

void Server::removeUser(const Socket &socket){
    std::set<User>::iterator it = _users.find(socket);
    if (it != _users.end())
        removeUser(it);
}

void Server::handleMessage(const Socket &client, const std::string &msg){
    int eof = std::char_traits<wchar_t>::eof();
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
            user = &*it;
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

        case CL_LOCATION:
        {
            double x, y;
            iss >> x >> del >> y >> del;
            if (del != ']')
                return;
            user->updateLocation(Point(x, y), *this);
            broadcast(SV_LOCATION, user->makeLocationCommand());
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
            for (std::string::const_iterator it = name.begin(); it != name.end(); ++it){
                if (*it < 'a' || *it > 'z') {
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

        case CL_COLLECT_BRANCH:
        {
            int serial;
            iss >> serial >> del;
            if (del != ']')
                return;
            std::set<BranchLite>::const_iterator it = _branches.find(serial);
            if (it == _branches.end()) {
                sendMessage(client, SV_DOESNT_EXIST);
            } else if (distance(user->location(), it->location) > ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
            } else {
                user->actionTargetBranch(&*it);
            }
            break;
        }

        case CL_COLLECT_TREE:
        {
            int serial;
            iss >> serial >> del;
            if (del != ']')
                return;
            std::set<TreeLite>::const_iterator it = _trees.find(serial);
            if (it == _trees.end()) {
                sendMessage(client, SV_DOESNT_EXIST);
            } else if (distance(user->location(), it->location) > ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
            } else {
                user->actionTargetTree(&*it);
            }
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg;
        }
    }
}

void Server::broadcast(MessageCode msgCode, const std::string &args){
    for (std::set<User>::const_iterator it = _users.begin(); it != _users.end(); ++it){
        sendMessage(it->socket(), msgCode, args);
    }
}

void Server::removeTree(size_t serial, User &user){
    // Give wood to user
    std::set<TreeLite>::iterator it = _trees.find(serial);
    int slot = user.giveItem(*_items.find(std::string("wood")));
    if (slot == User::INVENTORY_SIZE) {
        sendMessage(user.socket(), SV_INVENTORY_FULL);
        return;
    }
    sendMessage(user.socket(), SV_INVENTORY, makeArgs(slot, "wood", user.inventory(slot).second));
    // Ensure no other users are targeting this branch, as it will be removed.
    for (std::set<User>::iterator it = _users.begin(); it != _users.end(); ++it)
        if (&*it != &user && it->actionTargetTree()->serial == serial) {
            it->actionTargetTree(0);
            sendMessage(it->socket(), SV_DOESNT_EXIST);
        }
    // Remove tree if empty
    it->decrementWood();
    if (it->wood() == 0) {
        broadcast(SV_REMOVE_TREE, makeArgs(serial));
        _trees.erase(it);
    }
    saveData();
}

void Server::removeBranch(size_t serial, User &user){
    // Give wood to user
    int slot = user.giveItem(*_items.find(std::string("wood")));
    if (slot == User::INVENTORY_SIZE) {
        sendMessage(user.socket(), SV_INVENTORY_FULL);
        return;
    }
    sendMessage(user.socket(), SV_INVENTORY, makeArgs(slot, "wood", user.inventory(slot).second));
    // Ensure no other users are targeting this branch, as it will be removed.
    for (std::set<User>::iterator it = _users.begin(); it != _users.end(); ++it)
        if (&*it != &user && it->actionTargetBranch()->serial == serial) {
            it->actionTargetBranch(0);
            sendMessage(it->socket(), SV_DOESNT_EXIST);
        }
    // Remove branch
    std::set<BranchLite>::const_iterator it = _branches.find(serial);
    broadcast(SV_REMOVE_BRANCH, makeArgs(serial));
    _branches.erase(it);
    saveData();
}

bool Server::readUserData(User &user){
    std::string filename = std::string("Users/") + user.name() + ".usr";
    std::ifstream fs(filename.c_str());
    if (!fs.good()) // File didn't exist
        return false;
    
    // Location
    user.location(fs);

    // Inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::string itemID;
        int quantity;
        fs >> itemID >> quantity;
        user.inventory(i) = std::make_pair(itemID, quantity);
    }

    fs.close();
    return true;
}

void Server::writeUserData(const User &user) const{
    std::string filename = std::string("Users/") + user.name() + ".usr";
    std::ofstream fs(filename.c_str());
    
    // Location
    fs << user.location().x << ' ' << user.location().y << std::endl;

    // Inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        fs << user.inventory(i).first << ' ' << user.inventory(i).second << std::endl;
    }

    fs.close();
}


void Server::sendMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args) const{
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
    // Load data
    _items.insert(Item("wood", "wood", 5));

    Item i("axe", "wooden axe", 1);
    i.addClass("axe");
    _items.insert(i);

    _items.insert(Item("none", "none"));

    // Detect/load state
    do {
        if (cmdLineArgs.contains("new"))
            break;

        std::ifstream fs("World/map.dat");
        if (!fs.good())
            break;
        fs >> _mapX >> _mapY;
        _map = std::vector<std::vector<size_t> >(_mapX);
        for (size_t x = 0; x != _mapX; ++x)
            _map[x] = std::vector<size_t>(_mapY, 0);
        for (size_t y = 0; y != _mapY; ++y)
            for (size_t x = 0; x != _mapX; ++x)
                fs >> _map[x][y];
        if (!fs.good())
            break;
        fs.close();

        fs.open("World/branches.dat");
        if (!fs.good())
            break;
        int numBranches;
        fs >> numBranches;
        for (int i = 0; i != numBranches; ++i) {
            Point p;
            fs >> p.x >> p.y;
            _branches.insert(p);
        }
        if (!fs.good())
            break;
        fs.close();

        fs.open("World/trees.dat");
        if (!fs.good())
            break;
        int numTrees;
        fs >> numTrees;
        for (int i = 0; i != numTrees; ++i) {
            Point p;
            size_t wood;
            fs >> p.x >> p.y >> wood;
            _trees.insert(TreeLite(p, wood));
        }
        if (!fs.good())
            break;
        fs.close();

        return;
    } while (false);

    _debug << Color::YELLOW << "No/invalid world data detected; generating new world." << Log::endl;
    generateWorld();
}

void Server::saveData() const{
    std::ofstream fs("World/map.dat");
    fs << _mapX << '\n' << _mapY << '\n';
    for (size_t y = 0; y != _mapY; ++y){
        for (size_t x = 0; x != _mapX; ++x){
            fs << ' ' << _map[x][y];
        }
        fs << '\n';
    }
    fs.close();

    fs.open("World/branches.dat");
    fs << _branches.size() << '\n';
    for (std::set<BranchLite>::const_iterator it = _branches.begin(); it != _branches.end(); ++it) {
        fs << it->location.x << ' ' << it->location.y << '\n';
    }
    fs.close();

    fs.open("World/trees.dat");
    fs << _trees.size() << '\n';
    for (std::set<TreeLite>::const_iterator it = _trees.begin(); it != _trees.end(); ++it) {
        fs << it->location.x << ' ' << it->location.y << ' ' << it->wood() << '\n';
    }
    fs.close();
}

void Server::generateWorld(){
    _mapX = 20;
    _mapY = 20;

    // Grass by default
    _map = std::vector<std::vector<size_t> >(_mapX);
    for (size_t x = 0; x != _mapX; ++x){
        _map[x] = std::vector<size_t>(_mapY);
        for (size_t y = 0; y != _mapY; ++y)
            _map[x][y] = 0;
    }

    // Stone in circles
    for (int i = 0; i != 5; ++i) {
        size_t centerX = rand() % _mapX;
        size_t centerY = rand() % _mapY;
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                double dist = distance(Point(centerX, centerY), thisTile);
                if (dist <= 4)
                    _map[x][y] = 1;
            }
    }

    // Roads
    for (int i = 0; i != 2; ++i) {
        Point
            start(rand() % _mapX, rand() % _mapY),
            end(rand() % _mapX, rand() % _mapY);
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                double dist = distance(thisTile, start, end);
                if (dist <= 1)
                    _map[x][y] = 2;
            }
    }

    // River: randomish line
    Point
        start(rand() % _mapX, rand() % _mapY),
        end(rand() % _mapX, rand() % _mapY);
    for (size_t x = 0; x != _mapX; ++x)
        for (size_t y = 0; y != _mapY; ++y) {
            Point thisTile(x, y);
            if (y % 2 == 1)
                thisTile.x -= .5;
            double dist = distance(thisTile, start, end) + randDouble() * 2 - 1;
            if (dist <= 0.5)
                _map[x][y] = 3;
            else if (dist <= 2)
                _map[x][y] = 4;
        }

    for (int i = 0; i != 30; ++i)
        _branches.insert(mapRand());

    for (int i = 0; i != 10; ++i) {
        size_t wood = rand() % 3 + rand() % 3 + 6; // 5-9 wood per tree
        _trees.insert(TreeLite(mapRand(), wood));
    }
}

Point Server::mapRand() const{
    return Point(randDouble() * (_mapX - 0.5) * TILE_W,
                 randDouble() * _mapY * TILE_H);
}
