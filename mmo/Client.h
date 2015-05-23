#ifndef CLIENT_H
#define CLIENT_H

#include "Socket.h"

class Client{
public:
    Client();
    ~Client();
    void run();
    void draw();

private:
    Socket socket;
    SDL_Window *window;

    SDL_Surface
        *image,
        *screen;
};

#endif
