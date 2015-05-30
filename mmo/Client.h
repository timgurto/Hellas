#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <utility>
#include <map>

#include "Args.h"
#include "Log.h"
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

    bool _connected;

    static const int BUFFER_SIZE;

    std::pair<int, int> _location;

    std::map<std::string, std::pair<int, int> > _otherUserLocations;

    std::queue<std::string> _messages;

    mutable Log _debug;

    void checkSocket();
    void handleMessage(const std::string &msg);
};

#endif
