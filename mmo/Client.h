#ifndef CLIENT_H
#define CLIENT_H

#include "Socket.h"

class Client{
public:
    Client();
    ~Client();
    void run();

private:
    Socket socket;
    SDL_Window *window;
};

#endif
