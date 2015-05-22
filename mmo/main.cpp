#include <string>
#include "Socket.h"

int main(int argc, char** argv){
    Socket socket;
    if (argc > 1 && std::string(argv[1]) == "-server") {
        socket.runServer();
    } else {
        socket.runClient();
        socket.sendCommand("Test");
    }
}