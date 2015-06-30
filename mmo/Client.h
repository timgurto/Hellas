#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "Args.h"
#include "Branch.h"
#include "Entity.h"
#include "Item.h"
#include "Log.h"
#include "OtherUser.h"
#include "Point.h"
#include "Socket.h"
#include "messageCodes.h"

class Client{
public:
    Client();
    ~Client();
    void run();

    static const double MOVEMENT_SPEED; // per second

    static bool isClient;

    const Socket &socket() const;
    TTF_Font *defaultFont() const;

    inline const Entity &character() const { return _character; }
    inline const Point &offset() const { return _offset; }

private:
    static const Uint32 MAX_TICK_LENGTH;
    static const Uint32 SERVER_TIMEOUT; // How long the client will wait for a ping reply from the server
    static const Uint32 CONNECT_RETRY_DELAY; // How long to wait between retries at connecting to server
    static const Uint32 PING_FREQUENCY; // How often to test latency with each client
    static const Uint32 TIME_BETWEEN_LOCATION_UPDATES; // How often to send location updates to server (while moving)

    static const int
        SCREEN_X,
        SCREEN_Y;

    Texture _invLabel;
    Texture _tile[3];

    Entity _character; // Describes the user's character

    bool _loop;
    Socket _socket;
    TTF_Font *_defaultFont;
    std::string _username;

    Point _mouse; // Mouse position
    bool _mouseMoved;
    void checkMouseOver();

    void draw() const;
    void drawTile(size_t x, size_t y, int xLoc, int yLoc) const;
    void drawTooltip() const;
    Point _offset; // An offset for drawing, based on the character's location on the map.
    void updateOffset(); // Update the offset, when the character moves.

    std::string _partialMessage;

    Uint32 _time;
    Uint32 _timeElapsed; // Time between last two ticks
    Uint32 _lastPingReply; // The last time a ping reply was received from the server
    Uint32 _lastPingSent; // The last time a ping was sent to the server
    Uint32 _latency;
    Uint32 _timeSinceConnectAttempt;

    bool _invalidUsername; // Flag set if server refused username

    bool _loggedIn;
    bool _loaded; // Whether the client has sufficient information to begin

    static const size_t BUFFER_SIZE;

    Uint32 _timeSinceLocUpdate; // Time since a CL_LOCATION was sent
    bool _locationChanged;
    bool _tooltipNeedsRefresh; // Location has changed (local or official), and tooltip may have changed.

    // Game data
    std::set<Item> _items;

    // Information about the state of the world
    size_t _mapX, _mapY;
    std::vector<std::vector<size_t> > _map;
    std::vector<std::pair<std::string, size_t> > _inventory;
    std::map<std::string, OtherUser*> _otherUsers; // For lookup by name
    std::map<size_t, Branch*> _branches; // For lookup by serial

    Entity::set_t _entities;
    void removeEntity(Entity *const toRemove); // Remove from _entities, and delete pointer
    void setEntityLocation(Entity *entity, const Point &location); // Move the entity, and reorder it if necessary
    Entity *_currentMouseOverEntity;

    std::queue<std::string> _messages;

    mutable Log _debug;

    void checkSocket();
    void sendMessage(MessageCode msgCode, const std::string &args = "") const;
    void handleMessage(const std::string &msg);

    friend OtherUser;
};

#endif
