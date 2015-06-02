#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <map>

#include "Args.h"
#include "Log.h"
#include "Point.h"
#include "Socket.h"

class Client{
public:
    Client(const Args &args);
    ~Client();
    void run();
    void draw();

    //TODO make private; only public for server's random initial placement
    static const int SCREEN_WIDTH;
    static const int SCREEN_HEIGHT;

    static const Uint32 MAX_TICK_LENGTH;
    static const Uint32 SERVER_TIMEOUT; // How long the client will wait for a ping from the server
    static const Uint32 CONNECT_RETRY_DELAY; // How long to wait between retries at connecting to server

    static const double MOVEMENT_SPEED; // per second
    static const Uint32 TIME_BETWEEN_LOCATION_UPDATES;

private:
    const Args &_args; //comand-line args

    SDL_Window *_window;

    SDL_Surface
        *_image,
        *_screen;

    bool _loop;
    Socket _socket;
    TTF_Font *_defaultFont;
    std::string _username;

    Uint32 _time;
    Uint32 _timeElapsed; // Time between last two ticks
    Uint32 _lastPing;
    Uint32 _latency;
    Uint32 _timeSinceConnectAttempt;

    bool _invalidUsername; // Flag set if server refused username

    bool _connected;

    static const int BUFFER_SIZE;

    Point _location;
    Uint32 _timeSinceLocUpdate; // Time since a CL_LOCATION was sent
    bool _locationChanged;

    std::map<std::string, Point> _otherUserLocations;

    std::queue<std::string> _messages;

    mutable Log _debug;

    void checkSocket();
    void handleMessage(const std::string &msg);
};

#endif
