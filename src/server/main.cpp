// (C) 2015 Tim Gurto

#include <cassert>
#include <ctime>
#include <cstdlib>
#include <string>
#include <SDL.h>

#include "Server.h"
#include "../Args.h"
#include "../client/Renderer.h"
#include "../client/Texture.h"

Args cmdLineArgs; // MUST be defined before renderer
Renderer renderer; // MUST be defined after cmdLineArgs

int main(int argc, char* argv[]){
    cmdLineArgs.init(argc, argv);
    renderer.init();

    srand(static_cast<unsigned>(time(0)));

        Server server;
        server.run();

    assert(Texture::numTextures() == 0);
    return 0;
}
