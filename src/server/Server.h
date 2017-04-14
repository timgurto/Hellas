#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <queue>
#include <set>
#include <string>
#include <utility>

#include "CollisionChunk.h"
#include "ItemSet.h"
#include "LogConsole.h"
#include "NPC.h"
#include "Object.h"
#include "Recipe.h"
#include "ServerItem.h"
#include "Spawner.h"
#include "TerrainList.h"
#include "User.h"
#include "../Args.h"
#include "../messageCodes.h"
#include "../Socket.h"
#include "../Terrain.h"

class Server{
public:
    Server();
    ~Server();
    void run();

    static const ms_t CLIENT_TIMEOUT; // How much radio silence before we drop a client
    static const ms_t MAX_TIME_BETWEEN_LOCATION_UPDATES;

    static const px_t ACTION_DISTANCE; // How close a character must be to interact with an object
    static const px_t CULL_DISTANCE; // Users only get information within a circle with this radius.

    static const px_t TILE_W, TILE_H;
    const size_t mapX() const { return _mapX; }
    const size_t mapY() const { return _mapY; }

    static const Server &instance(){ return *_instance; }
    static LogConsole &debug(){ return *_debugInstance; }

    bool itemIsTag(const ServerItem *item, const std::string &tagName) const;

    const User &getUserByName(const std::string &username) const;

    mutable LogConsole _debug;

    enum SpecialSerial{
        INVENTORY = 0,
        GEAR = 1,

        STARTING_SERIAL
    };

    // Const Searches/queries
    size_t findTile(const Point &p) const; // Find the tile type at the specified location.
    std::pair<size_t, size_t> getTileCoords(const Point &p) const;
    size_t Server::getTileYCoord(double y) const;
    size_t Server::getTileXCoord(double x, size_t yTile) const;
    static Rect getTileRect(size_t x, size_t y);
    std::list<User*> findUsersInArea(Point loc, double squareRadius = CULL_DISTANCE) const;
    const ObjectType *findObjectTypeByName(const std::string &id) const; // Linear complexity
    std::set<char> nearbyTerrainTypes(const Rect &rect, double extraRadius = 0);

    // Checks whether the object is within range of the user.  If not, a relevant error message is
    // sent to the client.
    bool isObjectInRange(const Socket &client, const User &user, const Object *obj) const;

    const Terrain *terrainType(char index) const;

private:

    static Server *_instance;
    static LogConsole *_debugInstance;

    static const int MAX_CLIENTS;
    static const size_t BUFFER_SIZE = 1023;

    ms_t _time, _lastTime;

    Socket _socket;

    bool _loop;
    bool _running; // True while run() is being executed.

    // Messages
    std::queue<std::pair<Socket, std::string> > _messages;
    void sendMessage(const Socket &dstSocket, MessageCode msgCode,
                     const std::string &args = "") const;
    void broadcast(MessageCode msgCode, const std::string &args); // Send a command to all users
    void handleMessage(const Socket &client, const std::string &msg);
    void sendInventoryMessageInner(const User &user, size_t serial, size_t slot,
                                   const ServerItem::vect_t &itemVect) const;
    void sendInventoryMessage(const User &user, size_t slot, const Object &obj) const;
    void sendInventoryMessage(const User &user, size_t slot, size_t serial) const;
    void sendMerchantSlotMessage(const User &user, const Object &obj, size_t slot) const;
    void sendConstructionMaterialsMessage(const User &user, const Object &obj) const;
    void sendObjectInfo(const User &user, const Object &object) const;
    void sendUserInfo(const User &user, const User &userToDescribe) const;
    void sendNewBuildsMessage(const User &user, const std::set<std::string> &ids) const;
    void sendNewRecipesMessage(const User &user, const std::set<std::string> &ids) const;

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
    static Point newPlayerSpawnLocation;
    static double newPlayerSpawnRange;

    // Remove traces of a user who has disconnected.
    void removeUser(const Socket &socket);
    void removeUser(const std::set<User>::iterator &it);

    User::byX_t _usersByX; // This and below are for alerting users in a specific area.
    User::byY_t _usersByY;

    // World state
    typedef std::set<Object *, Object::compareSerial> objects_t;
    objects_t _objects;
    Object::byX_t _objectsByX; // This and below are for alerting users only to nearby objects.
    Object::byY_t _objectsByY;
    Object *findObject(size_t serial);
    Object *findObject(const Point &loc);

    void loadData(const std::string &path = "Data"); // Attempt to load data from files.
    bool _dataLoaded; // If false when run() is called, load default data.
    static void saveData(const objects_t &objects);
    void spawnInitialObjects();

    Point mapRand() const; // Return a random point on the map.
    size_t _mapX, _mapY; // Number of tiles in each dimension.
    std::vector<std::vector<char>> _map;

    // World data
    std::set<ServerItem> _items;
    std::set<Recipe> _recipes;
    std::set<const ObjectType *> _objectTypes;
    std::map<size_t, Spawner> _spawners;
    std::map<char, Terrain*> _terrainTypes;

    std::list<Object *> _objectsToRemove; // Emptied every tick.
    // Force all users to untarget an object
    void forceUntarget(const Object &obj, const User *userToExclude = nullptr); 
    void removeObject(Object &obj, const User *userToExclude = nullptr);
    void gatherObject (size_t serial, User &user);

    friend class Object;
    friend class User;
    friend class NPC;
    friend class Combatant;
    friend class ServerTestInterface;
    friend class Spawner;
    friend class ProgressLock;

    NPC &addNPC(const NPCType *type, const Point &location); 
    Object &addObject (const ObjectType *type, const Point &location, const User *owner = nullptr);
    Object &addObject (Object *newObj);

    // Collision detection
    static const px_t COLLISION_CHUNK_SIZE;
    CollisionGrid _collisionGrid;
    CollisionChunk &getCollisionChunk(const Point &p);
    std::list<CollisionChunk *> getCollisionSuperChunk(const Point &p);

    // thisObject = object to omit from collision detection (usually "this", to avoid self-collision)
    bool isLocationValid(const Point &loc, const ObjectType &type,
                         const Object *thisObject = nullptr); // Deduces allowed terrain from type
    bool isLocationValid(const Rect &rect, const Object *thisObject); // Deduces allowed terrain from thisObject
    bool isLocationValid(const Rect &rect, const TerrainList &allowedTerrain,
                         const Object *thisObject = nullptr);

    bool readUserData(User &user); // true: save data existed
    void writeUserData(const User &user) const;
    static const ms_t SAVE_FREQUENCY;
    ms_t _lastSave;
};

#endif
