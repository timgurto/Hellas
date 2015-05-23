#ifndef SERVER_H
#define SERVER_H

#include "Socket.h"

class Server{
public:
    Server();
    ~Server();
    void run();
    void runSocketServer();

private:
    static const int MAX_CLIENTS;
    static const int BUFFER_SIZE;

    Socket socket;
    SDL_Window *window;
};

#endif
