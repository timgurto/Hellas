#ifndef CLIENT_H
#define CLIENT_H

#include "Socket.h"

class Client{
public:
    Client();
    void run();

private:
    Socket socket;
};

#endif
