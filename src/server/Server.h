// (C) 2015 Tim Gurto

#ifndef SERVER_H
#define SERVER_H

#include <queue>
#include <set>
#include <string>
#include <utility>

#include "Item.h"
#include "Log.h"
#include "Object.h"
#include "User.h"
#include "../Args.h"
#include "../messageCodes.h"
#include "../Socket.h"

class Server{
public:
    Server();
    ~Server();
    void run();

    static const Uint32 CLIENT_TIMEOUT; // How much radio silence before we drop a client
    static const Uint32 MAX_TIME_BETWEEN_LOCATION_UPDATES;

    static const double MOVEMENT_SPEED; // per second
    static const int ACTION_DISTANCE; // How close a character must be to interact with an object

    static bool isServer;

    static const int TILE_W, TILE_H;
    const size_t mapX() const { return _mapX; }
    const size_t mapY() const { return _mapY; }

    bool itemIsClass(const Item *item, const std::string &className) const;

private:

    static const int MAX_CLIENTS;
    static const size_t BUFFER_SIZE;

    Uint32 _time, _lastTime;

    Socket _socket;

    bool _loop;

    // Messages
    std::queue<std::pair<Socket, std::string> > _messages;
    void sendMessage(const Socket &dstSocket, MessageCode msgCode,
                     const std::string &args = "") const;
    void broadcast(MessageCode msgCode, const std::string &args); // Send a command to all users
    void handleMessage(const Socket &client, const std::string &msg);

    // Clients
    // All connected sockets, including those without registered users
    std::set<Socket> _clientSockets;
    std::set<User> _users; // All connected users
    std::set<std::string> _usernames; // All connected users' names, for faster lookup of duplicates
    void checkSockets();
    /*
    Add the newly logged-in user
    This happens not once the client connects, but rather when a CL_I_AM message is received.
    */
    void addUser(const Socket &socket, const std::string &name);

    // Remove traces of a user who has disconnected.
    void removeUser(const Socket &socket);
    void removeUser(const std::set<User>::iterator &it);

    // World state
    std::set<Object> _objects;
    void loadData(); // Attempt to load data from files.
    static void saveData(const Server *server, const std::set<Object> &objects);
    void generateWorld(); // Randomly generate a new world.

    Point mapRand() const; // Return a random point on the map.
    size_t _mapX, _mapY; // Number of tiles in each dimension.
    std::vector<std::vector<size_t>> _map;
    size_t findTile(const Point &p) const; // Find the tile type at the specified location.

    // World data
    std::set<Item> _items;
    std::set<ObjectType> _objectTypes;

    mutable Log _debug;

    void gatherObject (size_t serial, User &user);
    friend void User::update(Uint32 timeElapsed, Server &server);
    friend void User::removeMaterials(const Item &item, Server &server);
    friend void User::cancelAction(Server &server);

    friend size_t User::giveItem(const Item *item, size_t quantity, const Server &server);

    void addObject (const ObjectType *type, const Point &location, const User *owner = 0);

    bool readUserData(User &user); // true: save data existed
    void writeUserData(const User &user) const;
    static const Uint32 SAVE_FREQUENCY;
    Uint32 _lastSave;
};

#endif
