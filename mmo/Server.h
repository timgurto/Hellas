#ifndef SERVER_H
#define SERVER_H

#include "Socket.h"

class Server{
public:
    Server();
    ~Server();
    void run();

private:
    Socket socket;
    SDL_Window *window;
};

#endif
