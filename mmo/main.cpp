#include <string>
#include "socket.h"

int main(int argc, char** argv){
    if (argc > 1 && std::string(argv[1]) == "-server") {
        server();
    } else {
        client();
        sendCommand("1");
        sendCommand("2");
        sendCommand("3");
    }
}