#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <set>
#include <utility>
#include <queue>
#include <string>

#include "Socket.h"

class Server{
public:
    Server();
    ~Server();
    void run();
    void runSocketServer();

private:
    static const int MAX_CLIENTS;
    static const int BUFFER_SIZE;

    Socket _socket;
    SDL_Window *_window;
    SDL_Surface *_screen;
    SDL_Thread *_socketThreadID;

    bool _loop;
    bool _socketLoop;

    std::set<SOCKET> _clientSockets;

    std::queue<std::pair<SOCKET, std::string> > _messages;

    std::map<SOCKET, std::pair<int, int> > _userLocations;

    void addNewUser(SOCKET socket);

    // Send a user's location to all users
    void sendUserLocation(SOCKET socket);

    void handleMessage(SOCKET user, std::string msg);
};

#endif
