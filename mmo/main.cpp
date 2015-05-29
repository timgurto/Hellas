#include <ctime>
#include <cstdlib>
#include <string>
#include <SDL.h>

#include "Args.h"
#include "Client.h"
#include "Server.h"

int main(int argc, char* argv[]){
    Args args(argc, argv);

    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return 1;
    ret = TTF_Init();
    if (ret < 0)
        return 1;
    srand(static_cast<unsigned>(time(0)));

    if (args.contains("server")){
        Server server(args);
        server.run();
    } else {
        Client client(args);
        client.run();
    }
    
    TTF_Quit();
    SDL_Quit();

    return 0;
}
