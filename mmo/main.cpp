#include <string>
#include <SDL.h>

#include "Client.h"
#include "Server.h"

int main(int argc, char* args[]){
    if (argc > 1 && std::string(args[1]) == "-server") {
        Server server;
        server.run();
    } else {
        Client client;
        client.run();
    }
    return 0;
}
