#include <string>
#include <SDL.h>

#include "Client.h"
#include "Server.h"

int main(int argc, char* args[]){

    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return 1;
    ret = TTF_Init();
    if (ret < 0)
        return 1;

    if (argc > 1 && std::string(args[1]) == "-server") {
    //if (argc == 1) { /* Swap server/client, for debugging */
        Server server;
        server.run();
    } else {
        Client client;
        client.run();
    }
    
    TTF_Quit();
    SDL_Quit();

    return 0;
}
