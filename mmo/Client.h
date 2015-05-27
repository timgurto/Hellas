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
    bool _socketLoop;
    Socket _socket;
    SDL_Thread *_socketThreadID;

    std::pair<int, int> _location;

    std::map<SOCKET, std::pair<int, int> > _otherUserLocations;

    std::queue<std::string> _messages;

    Log _debug;

    void checkSocket();
    void handleMessage(std::string msg);
};

#endif
