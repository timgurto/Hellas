#include <string>
#include <SDL.h>
#include <SDL_main.h>

#include "Socket.h"
#include "Server.h"

#undef main

int main(int argc, char* args[]){
    Socket socket;
    if (argc > 1 && std::string(args[1]) == "-server") {
        Server server;
        server.run();
    } else {
        socket.runClient();
        socket.sendCommand("Test");
    }
    std::cout << "Execution is here" << std::endl;
    return 0;
}
