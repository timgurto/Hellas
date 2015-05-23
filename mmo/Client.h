#ifndef CLIENT_H
#define CLIENT_H

#include <queue>

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

    Socket socket;
    SDL_Window *window;

    SDL_Surface
        *image,
        *screen;

    std::queue<std::string> _messages;
};

#endif
