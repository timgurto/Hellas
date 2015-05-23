#include <windows.h>
#include <SDL.h>

#include "Client.h"

Client::Client():
window(0){
    socket.runClient();

    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return;

    window = SDL_CreateWindow("Client", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window)
        return;
}

Client::~Client(){
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

void Client::run(){
    socket.sendCommand("Test");

    while (true)
        ;
}
