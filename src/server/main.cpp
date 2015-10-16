// (C) 2015 Tim Gurto

#include <cassert>
#include <ctime>
#include <cstdlib>
#include <string>
#include <SDL.h>

#include "Server.h"
#include "../Args.h"
#include "../client/Renderer.h"

Args cmdLineArgs;

int main(int argc, char* argv[]){
    cmdLineArgs.init(argc, argv);

    srand(static_cast<unsigned>(time(0)));

    Server server;
    server.run();

    return 0;
}
