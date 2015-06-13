#include <SDL.h>
#include <sstream>
#include <fstream>

#include "Client.h" //TODO remove; only here for random initial placement
#include "Socket.h"
#include "Server.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

const int Server::MAX_CLIENTS = 20;
const size_t Server::BUFFER_SIZE = 1023;

const Uint32 Server::CLIENT_TIMEOUT = 10000;

const Uint32 Server::SAVE_FREQUENCY = 1000;

const int Server::ACTION_DISTANCE = 20;

bool Server::isServer = false;

SDL_Renderer *Server::screen = 0;

Server::Server(const Args &args):
_args(args),
_loop(true),
_debug(100),
_socket(),
_time(SDL_GetTicks()),
_lastSave(_time){
    isServer = true;

    _debug << args << Log::endl;
    Socket::debug = &_debug;

    int screenX = _args.contains("left") ?
                  _args.getInt("left") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenY = _args.contains("top") ?
                  _args.getInt("top") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenW = _args.contains("width") ?
                  _args.getInt("width") :
                  800;
    int screenH = _args.contains("height") ?
                  _args.getInt("height") :
                  600;
    _window = SDL_CreateWindow("Server", screenX, screenY, screenW, screenH, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    screen = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);

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
    if (screen)
        SDL_DestroyRenderer(screen);
    if (_window)
        SDL_DestroyWindow(_window);
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
    SDL_SetRenderDrawColor(screen, 0, 0, 0, 0xff); // black
    SDL_RenderFillRect(screen, 0);
    _debug.draw(screen);
    SDL_RenderPresent(screen);
}

void Server::addUser(const Socket &socket, const std::string &name){
    User newUser(name, 0, socket);
    bool userExisted = readUserData(newUser);
    if (!userExisted) {
        newUser.location(Point(rand() % (Client::SCREEN_WIDTH - 20),
                               rand() % (Client::SCREEN_HEIGHT - 40)));
        _debug << "New";
    } else {
        _debug << "Existing";
    }
    _debug << " user, " << name << " has logged in." << Log::endl;
    _usernames.insert(name);

    // Send welcome message
    sendMessage(socket, SV_WELCOME);

    // Send new user everybody else's location
    for (std::set<User>::const_iterator it = _users.begin(); it != _users.end(); ++it)
        sendMessage(newUser.socket(), SV_LOCATION, it->makeLocationCommand());

    // Send him branch locations
    for (std::set<Branch>::const_iterator it = _branches.begin(); it != _branches.end(); ++it)
        sendMessage(newUser.socket(), SV_BRANCH, makeArgs(it->serial(), it->location().x, it->location().y));

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
            user->updateLocation(Point(x, y));
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
            std::set<Branch>::const_iterator it = _branches.find(serial);
            if (it == _branches.end()) {
                sendMessage(client, SV_DOESNT_EXIST);
            } else if (distance(user->location(), it->location()) > ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
            } else {
                // Give wood to user
                int slot = user->giveItem(*_items.find(std::string("wood")));
                if (slot == User::INVENTORY_SIZE) {
                    sendMessage(client, SV_INVENTORY_FULL);
                    break;
                }
                sendMessage(client, SV_INVENTORY, makeArgs(slot, "wood", user->inventory(slot).second));
                // Remove branch
                broadcast(SV_REMOVE_BRANCH, makeArgs(serial));
                _branches.erase(it);
                saveData();
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
    std::ifstream fs("World/branches.dat");
    if (fs.good()) {
        // Load data
        int numBranches;
        fs >> numBranches;
        for (int i = 0; i != numBranches; ++i) {
            Point p;
            fs >> p.x >> p.y;
            _branches.insert(p);
        }
        fs.close();

    } else {
        // Create new random world
        for (int i = 0; i != 30; ++i)
            _branches.insert(Point(rand() % (Client::SCREEN_WIDTH - 20),
                                  (rand() % Client::SCREEN_HEIGHT - 20)));
    }

    _items.insert(Item("wood", "wood", 5));
    _items.insert(Item("none", "none"));
}

void Server::saveData(){
    std::ofstream fs("World/branches.dat");
    fs << _branches.size() << std::endl;
    for (std::set<Branch>::const_iterator it = _branches.begin(); it != _branches.end(); ++it) {
        fs << it->location().x << ' ' << it->location().y << std::endl;
    }
    fs.close();
}
