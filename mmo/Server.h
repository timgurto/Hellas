#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <set>
#include <utility>
#include <queue>
#include <string>

#include "Log.h"
#include "Socket.h"

class Server{
public:
    Server();
    ~Server();
    void run();

private:
    static const int MAX_CLIENTS;
    static const int BUFFER_SIZE;

    Socket _socket;
    SDL_Window *_window;
    SDL_Surface *_screen;

    bool _loop;

    std::set<SOCKET> _clientSockets;

    std::queue<std::pair<SOCKET, std::string> > _messages;

    std::map<SOCKET, std::pair<int, int> > _userLocations;

    Log _debug;

    void addNewUser(SOCKET socket);

    // Send a user's location to all users
    void sendUserLocation(SOCKET socket);

    void checkSockets();
    void handleMessage(SOCKET user, std::string msg);
};

#endif
