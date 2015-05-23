#include <windows.h>
#include <SDL.h>

#include "Server.h"

DWORD WINAPI runSocketServer(LPVOID socket){
    ((Socket*)socket)->runServer();
    return 0;
}

Server::Server(){
    DWORD socketThreadID;
    CreateThread(0, 0, &runSocketServer, &socket, 0, &socketThreadID);
    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return;

    window = SDL_CreateWindow("Server", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window)
        return;
}

Server::~Server(){
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

void Server::run(){

    while (true)
        ;
}
