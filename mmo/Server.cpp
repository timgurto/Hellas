#include <windows.h>

#include "Server.h"

DWORD WINAPI runSocketServer(LPVOID socket){
    ((Socket*)socket)->runServer();
    return 0;
}

Server::Server(){
    DWORD socketThreadID;
    CreateThread(0, 0, &runSocketServer, &socket, 0, &socketThreadID);
}

void Server::run(){
    while (true)
        ;
}
