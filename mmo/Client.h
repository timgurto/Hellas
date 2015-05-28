#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <utility>
#include <map>

#include "Log.h"
#include "Socket.h"

class Client{
public:
    Client();
    ~Client();
    void run();
    void draw();

private:
    static const int BUFFER_SIZE;

    SDL_Window *_window;

    SDL_Surface
        *_image,
        *_screen;
    bool _loop;
    Socket _socket;
    TTF_Font *_defaultFont;
    std::string _playerName;

    std::pair<int, int> _location;

    std::map<SOCKET, std::pair<int, int> > _otherUserLocations;

    std::queue<std::string> _messages;

    Log _debug;

    void checkSocket();
    void handleMessage(const std::string &msg);
};

#endif
