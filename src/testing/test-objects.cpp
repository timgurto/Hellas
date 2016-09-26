// (C) 2016 Tim Gurto

#include <cstdlib>
#include <SDL.h>
#include <thread>

#include "Test.h"
#include "../client/Client.h"
#include "../server/Server.h"

TEST("Create server and client objects")
    Server s;
    Client c;
    return true;
TEND

template<typename T>
static void start(T *t){
    t->run();
}

TEST("Start server")
    Server server;
    std::thread(start<Server>, &server).detach();
    return true;
TEND
