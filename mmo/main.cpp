#include <string>

#include "Socket.h"
#include "Server.h"

int main(int argc, char** argv){
    Socket socket;
    if (argc > 1 && std::string(argv[1]) == "-server") {
        Server server;
        server.run();
    } else {
        socket.runClient();
        socket.sendCommand("Test");
    }
    std::cout << "Execution is here" << std::endl;
}
