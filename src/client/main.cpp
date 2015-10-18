// (C) 2015 Tim Gurto

#include <cassert>
#include <ctime>
#include <cstdlib>
#include <string>
#include <SDL.h>

#include "Client.h"
#include "Renderer.h"
#include "Texture.h"
#include "../Args.h"

Args cmdLineArgs; // MUST be defined before renderer
Renderer renderer; // MUST be defined after cmdLineArgs

int main(int argc, char* argv[]){
    cmdLineArgs.init(argc, argv);
    renderer.init();

    srand(static_cast<unsigned>(time(0)));

    {
        Client client;
        client.run();
    }

    assert(Texture::numTextures() == 0);
    return 0;
}
