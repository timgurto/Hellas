#include <windows.h>
#include <SDL.h>
#include <string>

#include "Client.h"

Client::Client():
window(0),
image(0),
screen(0){
    socket.runClient();

    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return;

    window = SDL_CreateWindow("Client", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window)
        return;
    screen = SDL_GetWindowSurface(window);

    image = SDL_LoadBMP("Images/man.bmp");
    std::string err = SDL_GetError();

}

Client::~Client(){
    if (image)
        SDL_FreeSurface(image);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

void Client::run(){
    socket.sendCommand("Test");

    if (!window || !image)
        return;

    draw();

    SDL_Delay(2000);
}

void Client::draw(){
    SDL_BlitSurface(image, 0, screen, 0);
    SDL_UpdateWindowSurface(window);
}