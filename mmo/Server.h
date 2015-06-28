#ifndef SERVER_H
#define SERVER_H

#include <set>
#include <utility>
#include <queue>
#include <string>

#include "Args.h"
#include "BranchLite.h"
#include "Item.h"
#include "Log.h"
#include "Socket.h"
#include "User.h"
#include "messageCodes.h"

class Server{
public:
    Server();
    ~Server();
    void run();

    static const Uint32 CLIENT_TIMEOUT; // How much radio silence before we drop a client

    static const int ACTION_DISTANCE; // How close a character must be to interact with an object

    static bool isServer;

    static const size_t TILE_W, TILE_H;
    inline const size_t mapX() const { return _mapX; }
    inline const size_t mapY() const { return _mapY; }

private:

    static const int MAX_CLIENTS;
    static const size_t BUFFER_SIZE;

    Uint32 _time;

    Socket _socket;

    bool _loop;

    // Messages
    std::queue<std::pair<Socket, std::string> > _messages;
    void sendMessage(const Socket &dstSocket, MessageCode msgCode, const std::string &args = "") const;
    void broadcast(MessageCode msgCode, const std::string &args); // Send a command to all users
    void handleMessage(const Socket &client, const std::string &msg);

    // Clients
    std::set<Socket> _clientSockets; // All connected sockets, including those without registered users
    std::set<User> _users; // All connected users
    std::set<std::string> _usernames; // All connected users' names, for faster lookup of duplicates
    void checkSockets();
    // Add the newly logged-in user; this happens not once the client connects, but rather when a CL_I_AM message is received.
    void addUser(const Socket &socket, const std::string &name);

    // Remove traces of a user who has disconnected.
    void removeUser(const Socket &socket);
    void removeUser(std::set<User>::iterator &it);

    // World state
    std::set<BranchLite> _branches;
    void loadData(); // Attempt to load data from files.
    void saveData() const;
    void generateWorld(); // Randomly generate a new world.
    Point mapRand() const; // Return a random point on the map.
    size_t _mapX, _mapY; // Number of tiles in each dimension
    std::vector<std::vector<size_t>> _map;

    // World data
    std::set<Item> _items;

    mutable Log _debug;

    void draw() const;

    bool readUserData(User &user); // true: save data existed
    void writeUserData(const User &user) const;
    static const Uint32 SAVE_FREQUENCY;
    Uint32 _lastSave;
};

#endif
