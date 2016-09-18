// (C) 2016 Tim Gurto

#include <cstdlib>
#include <SDL.h>

#include "Test.h"
#include "../client/Client.h"
#include "../server/Server.h"

TEST("Run client and server")
    Server s;
    Client c;
    return true;
TEND
