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
    Client(const Args &args);
    ~Client();
    void run();
    void draw();

    //TODO make private; only public for server's random initial placement
    static const int SCREEN_WIDTH;
    static const int SCREEN_HEIGHT;

    static const double MOVEMENT_SPEED; // per second

private:
    const Args &_args; //comand-line args
    
    static const Uint32 MAX_TICK_LENGTH;
    static const Uint32 SERVER_TIMEOUT; // How long the client will wait for a ping reply from the server
    static const Uint32 CONNECT_RETRY_DELAY; // How long to wait between retries at connecting to server
    static const Uint32 PING_FREQUENCY; // How often to test latency with each client
    static const Uint32 TIME_BETWEEN_LOCATION_UPDATES; // How often to send location updates to server (while moving)

    SDL_Window *_window;

    SDL_Surface
        *_screen,
        *_invLabel;

    Entity _character; // Describes the user's character

    bool _loop;
    Socket _socket;
    TTF_Font *_defaultFont;
    std::string _username;
    Point _mouse; // Mouse position

    std::string _partialMessage;

    Uint32 _time;
    Uint32 _timeElapsed; // Time between last two ticks
    Uint32 _lastPingReply; // The last time a ping reply was received from the server
    Uint32 _lastPingSent; // The last time a ping was sent to the server
    Uint32 _latency;
    Uint32 _timeSinceConnectAttempt;

    bool _invalidUsername; // Flag set if server refused username

    bool _connected;
    bool _loaded; // Whether the client has sufficient information to begin

    static const size_t BUFFER_SIZE;

    Uint32 _timeSinceLocUpdate; // Time since a CL_LOCATION was sent
    bool _locationChanged;

    // Game data
    std::set<Item> _items;

    // Information about the state of the world
    std::vector<std::pair<std::string, size_t> > _inventory;
    std::map<std::string, OtherUser> _otherUsers;
    std::set<Branch> _branches;

    struct EntityCompare{
        bool operator()(const Entity *lhs, const Entity *rhs) const{ return *lhs < *rhs; }
    };
    std::set<const Entity *, EntityCompare> _entities;

    // Change an Entity's location, and ensure _entities remains ordered
    void setEntityLocation(Entity &entity, const Point &newLocation);

    std::queue<std::string> _messages;

    mutable Log _debug;

    void checkSocket();
    void sendMessage(MessageCode msgCode, const std::string &args = "") const;
    void handleMessage(const std::string &msg);
};

#endif
