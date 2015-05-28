#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <set>
#include <utility>
#include <queue>
#include <string>

#include "Log.h"
#include "Socket.h"
#include "User.h"

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

    std::list<User> _users;

    mutable Log _debug;

    // Add the newly logged-in user; this happens not once the client connects, but rather when a CL_I_AM message is received.
    void addNewUser(SOCKET socket, const std::string &name);

    // Send a command to a specific user
    void sendCommand(const User &dstUser, const std::string &msg) const;

    // Send a command to all users
    void broadcast(const std::string &msg) const;

    // Send a user's location to all users
    void sendUserLocation(const User &user) const;

    void checkSockets();
    void handleMessage(SOCKET client, const std::string &msg);

    User *getUserBySocket(SOCKET socket);
};

#endif
