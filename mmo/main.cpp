#include <string>
#include <windows.h>

#include "Socket.h"

DWORD WINAPI f(LPVOID socket){
    ((Socket*)socket)->runServer();
    return 0;
}

int main(int argc, char** argv){
    Socket socket;
    if (argc > 1 && std::string(argv[1]) == "-server") {
        DWORD socketThreadID;
        HANDLE socketThreadHandle = CreateThread(0, 0, &f, &socket, 0, &socketThreadID);
    } else {
        socket.runClient();
        socket.sendCommand("Test");
    }
    std::cout << "Execution is here" << std::endl;
}
