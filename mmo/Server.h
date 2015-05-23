#ifndef SERVER_H
#define SERVER_H

#include "Socket.h"

class Server{
public:
    Server();
    void run();

private:
    Socket socket;
};

#endif