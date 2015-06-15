#include <ctime>
#include <cstdlib>
#include <string>
#include <SDL.h>

#include "Args.h"
#include "Client.h"
#include "Server.h"

Args cmdLineArgs;

int main(int argc, char* argv[]){
    cmdLineArgs.init(argc, argv);

    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return 1;
    ret = TTF_Init();
    if (ret < 0)
        return 1;
    srand(static_cast<unsigned>(time(0)));

    if (cmdLineArgs.contains("server")){
        Server server;
        server.run();
    } else {
        Client client;
        client.run();
    }

    return 0;
}
