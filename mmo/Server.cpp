#include <SDL.h>
#include <sstream>

#include "Socket.h"
#include "Server.h"
#include "messageCodes.h"

const int Server::MAX_CLIENTS = 10;
const int Server::BUFFER_SIZE = 100;

Server::Server():
_loop(true),
_debug(30),
_socket(&_debug){

    _window = SDL_CreateWindow("Server", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    _screen = SDL_GetWindowSurface(_window);

    _debug("Server initialized");

    // Socket details
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    _socket.bind(serverAddr);
    _socket.listen();

    _debug("Listening for connections");
}

Server::~Server(){
    if (_window)
        SDL_DestroyWindow(_window);
}

void Server::checkSockets(){
    // Populate socket list with active sockets
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.raw(), &readFDs);
    for (std::set<SOCKET>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it)
        FD_SET(*it, &readFDs);

    // Poll for activity
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }

    // Activity on server socket: new connection
    if (FD_ISSET(_socket.raw(), &readFDs)) {
        if (_clientSockets.size() == MAX_CLIENTS)
           _debug("No room for additional clients; all slots full");
        else {
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.raw(), (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            if (tempSocket == SOCKET_ERROR) {
                _debug << "Error accepting connection: " << WSAGetLastError() << Log::endl;
            } else {
                _debug << "Connection accepted: "
                       << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port)
                       << ", socket number = " << tempSocket << Log::endl;
                _clientSockets.insert(tempSocket);
            }
        }
    }

    // Activity on client socket: message received or client disconnected
    for (std::set<SOCKET>::iterator it = _clientSockets.begin(); it != _clientSockets.end();) {
        if (FD_ISSET(*it, &readFDs)) {
            sockaddr_in clientAddr;
            getpeername(*it, (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            static char buffer[BUFFER_SIZE+1];
            int charsRead = recv(*it, buffer, BUFFER_SIZE, 0);
            if (charsRead == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAECONNRESET) {
                    // Client disconnected
                    _debug << "Client " << *it << " disconnected" << Log::endl;
                    closesocket(*it);
                    _clientSockets.erase(it++);
                    continue;
                } else {
                    _debug << "Error receiving message: " << err << Log::endl;
                }
            } else if (charsRead == 0) {
                // Client disconnected
                _debug << "Client " << *it << " disconnected" << Log::endl;
                closesocket(*it);
                _clientSockets.erase(it++);
                continue;
            } else {
                // Message received
                buffer[charsRead] = '\0';
                _debug << "Message received from client " << *it << ": " << buffer << Log::endl;
                _messages.push(std::make_pair(*it, std::string(buffer)));
            }
        }
        ++it;
    }
}

void Server::run(){
    while (_loop) {
        // Deal with any messages from the server
        if (!_messages.empty()){
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

        SDL_FillRect(_screen, 0, SDL_MapRGB(_screen->format, 0, 0, 0));
        _debug.draw(_screen);
        SDL_UpdateWindowSurface(_window);

        checkSockets();

        SDL_Delay(10);
    }
}

void Server::addNewUser(SOCKET socket, const std::string &name){
    _userSockets[name] = socket;
    _socketUsers[socket] = name;

    // Give new user a location
    _userLocations[name] = std::make_pair(500, 200);

    sendUserLocation(name);

    // Send new user everybody else's location
    for (std::set<SOCKET>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it){
        if (*it != socket &&
            _socketUsers.find(*it) != _socketUsers.end() &&
            _userLocations.find(_socketUsers[*it]) != _userLocations.end()) {
            std::ostringstream oss;
            oss << '[' << SV_OTHER_LOCATION << ','
                << *it << ','
                << _userLocations[_socketUsers[*it]].first << ','
                << _userLocations[_socketUsers[*it]].second << ']';
            Socket::sendMessage(socket, oss.str());
        }
    }

    _debug << "New user, " << name << " has been registered." << Log::endl;
}

void Server::handleMessage(SOCKET user, const std::string &msg){
    int eof = std::char_traits<wchar_t>::eof();
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    bool sendLocation = false;
    std::istringstream iss(msg);
    while (iss.peek() == '[') {
        iss >> del >> msgCode >> del;
        
        // Discard message if this client has not yet sent CL_I_AM
        if (_socketUsers.find(user) == _socketUsers.end() && msgCode != CL_I_AM)
            continue;
        const std::string &playerName = _socketUsers[user];

        switch(msgCode) {

        case CL_MOVE_UP:
            if (del != ']')
                return;
            _userLocations[playerName].second -= 20;
            sendLocation = true;
            break;

        case CL_MOVE_DOWN:
            if (del != ']')
                return;
            _userLocations[playerName].second += 20;
            sendLocation = true;
            break;

        case CL_MOVE_LEFT:
            if (del != ']')
                return;
            _userLocations[playerName].first -= 20;
            sendLocation = true;
            break;

        case CL_MOVE_RIGHT:
            if (del != ']')
                return;
            _userLocations[playerName].first += 20;
            sendLocation = true;
            break;

        case CL_I_AM:
        {
            iss.get(buffer, BUFFER_SIZE, ']');
            std::string name(buffer);
            iss >> del;
            if (del != ']')
                return;
            addNewUser(user, name);
            break;
        }

        default:
            ;
        }
    }

    if (sendLocation) {
        sendUserLocation(_socketUsers[user]);
    }

}

void Server::sendCommand(const std::string &name, const std::string &msg) const{
    std::map<std::string, SOCKET>::const_iterator it = _userSockets.find(name);
    if (it != _userSockets.end())
        Socket::sendMessage(it->second, msg, &_debug);
}

void Server::sendUserLocation(const std::string &userName) const{
    // Client himself
    std::map<std::string, std::pair<int, int>>::const_iterator it = _userLocations.find(userName);
    if (it == _userLocations.end())
        return;
    std::pair<int, int> loc = it->second;
    std::ostringstream oss;
    oss << '[' << SV_LOCATION << ',' << loc.first << ',' << loc.second << ']';
    sendCommand(userName, oss.str());

    // Other clients
    std::map<std::string, SOCKET>::const_iterator it2 = _userSockets.find(userName);
    if (it2 == _userSockets.end())
        return;
    SOCKET socket = it2->second;
    oss.str("");
    oss << '[' << SV_OTHER_LOCATION << ',' << socket << ',' << loc.first << ',' << loc.second << ']';
    for (std::set<SOCKET>::const_iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it)
        if (*it != socket)
            Socket::sendMessage(*it, oss.str());
}
