// (C) 2015-2016 Tim Gurto

#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <queue>
#include <set>
#include <string>
#include <utility>

#include "CollisionChunk.h"
#include "Item.h"
#include "ItemSet.h"
#include "LogConsole.h"
#include "Object.h"
#include "Recipe.h"
#include "Terrain.h"
#include "User.h"
#include "../Args.h"
#include "../messageCodes.h"
#include "../Socket.h"

class Server{
public:
    Server();
    ~Server();
    void run();

    static const ms_t CLIENT_TIMEOUT; // How much radio silence before we drop a client
    static const ms_t MAX_TIME_BETWEEN_LOCATION_UPDATES;

    static const double MOVEMENT_SPEED; // per second
    static const px_t ACTION_DISTANCE; // How close a character must be to interact with an object

    static const px_t TILE_W, TILE_H;
    const size_t mapX() const { return _mapX; }
    const size_t mapY() const { return _mapY; }

    static const Server &instance(){ return *_instance; }
    static LogConsole &debug(){ return *_debugInstance; }

    bool itemIsClass(const Item *item, const std::string &className) const;

    const User &getUserByName(const std::string &username) const;

    mutable LogConsole _debug;

private:

    static Server *_instance;
    static LogConsole *_debugInstance;

    static const int MAX_CLIENTS;
    static const size_t BUFFER_SIZE = 1023;

    ms_t _time, _lastTime;

    Socket _socket;

    bool _loop;

    // Messages
    std::queue<std::pair<Socket, std::string> > _messages;
    void sendMessage(const Socket &dstSocket, MessageCode msgCode,
                     const std::string &args = "") const;
    void broadcast(MessageCode msgCode, const std::string &args); // Send a command to all users
    void handleMessage(const Socket &client, const std::string &msg);
    void sendInventoryMessage(const User &user, size_t serial, size_t slot) const;
    void sendMerchantSlotMessage(const User &user, const Object &obj, size_t slot) const;

    // Clients
    // All connected sockets, including those without registered users
    std::set<Socket> _clientSockets;
    std::set<User> _users; // All connected users
    // Pointers to all connected users, ordered by name for faster lookup
    std::map<std::string, const User *> _usersByName;
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
    static void saveData(const std::set<Object> &objects);
    void generateWorld(); // Randomly generate a new world.

    Point mapRand() const; // Return a random point on the map.
    size_t _mapX, _mapY; // Number of tiles in each dimension.
    std::vector<std::vector<size_t>> _map;
    size_t findTile(const Point &p) const; // Find the tile type at the specified location.
    std::pair<size_t, size_t> getTileCoords(const Point &p) const;

    // World data
    std::vector<Terrain> _terrain;
    std::set<Item> _items;
    std::set<Recipe> _recipes;
    std::set<ObjectType> _objectTypes;

    void removeObject(Object &obj,
                      const User *userToExclude = nullptr); // Optionally skip telling a user
    void gatherObject (size_t serial, User &user);
    friend void User::update(ms_t timeElapsed);
    friend void User::removeItems(const ItemSet &items);
    friend void User::cancelAction();
    friend void User::updateLocation(const Point &dest);

    friend size_t User::giveItem(const Item *item, size_t quantity);

    friend bool User::hasTool(const std::string &className) const;
    friend bool User::hasTools(const std::set<std::string> &classes) const;

    friend void Object::removeItems(const ItemSet &items);
    friend void Object::giveItem(const Item *item, size_t qty);

    void addObject (const ObjectType *type, const Point &location, const User *owner = nullptr);

    // Checks whether the object exists, and is within range of the user.  If not, a relevant error
    // message is sentg to the client.
    bool isValidObject(const Socket &client, const User &user,
                       const std::set<Object>::const_iterator &it) const;

    // Collision detection
    static const px_t COLLISION_CHUNK_SIZE;
    CollisionGrid _collisionGrid;
    CollisionChunk &getCollisionChunk(const Point &p);
    std::list<CollisionChunk *> getCollisionSuperChunk(const Point &p);
    bool isLocationValid(const Point &loc, const ObjectType &type,
                         const Object *thisObject = nullptr, const User *thisUser = nullptr);

    bool readUserData(User &user); // true: save data existed
    void writeUserData(const User &user) const;
    static const ms_t SAVE_FREQUENCY;
    ms_t _lastSave;
};

#endif
