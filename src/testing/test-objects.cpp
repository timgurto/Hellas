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

TEST("Start and stop server")
    LogConsole log;
    Server server;
    Socket::debug = &log;
    std::thread([& server](){ server.run(); }).detach(); // Run server loop in new thread
    while (!server.running()) ; // Wait for server to begin running
    server.loop(false); // Stop server
    while (server.running()) ; // Wait for server to finish
    return true;
TEND

TEST("Multiple calls to Server::loadData()")
    Server s;
    s.loadData("testing/data/basic_rock");
    s.loadData("testing/data/basic_rock");
    return s._objectTypes.size() == 1;
TEND
