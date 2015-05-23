#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <utility>

#include "Socket.h"

class Client{
public:
    Client();
    ~Client();
    void runSocketClient();
    void run();
    void draw();

private:
    static const int BUFFER_SIZE;

    SDL_Window *window;

    SDL_Surface
        *image,
        *screen;
    bool _loop;
    Socket _socket;

    std::pair<int, int> _location;

    std::queue<std::string> _messages;

    void handleMessage(std::string msg);
};

#endif
