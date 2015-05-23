#include <windows.h>
#include <SDL.h>
#include <string>

#include "Client.h"

const int Client::BUFFER_SIZE = 100;

DWORD WINAPI startSocketClient(LPVOID client){
    ((Client*)client)->runSocketClient();
    return 0;
}

Client::Client():
window(0),
image(0),
screen(0){
    DWORD socketThreadID;
    CreateThread(0, 0, &startSocketClient, this, 0, &socketThreadID);

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

void Client::runSocketClient(){
    Socket s;
    // Server details
    sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);

    // Connect to server
    if (connect(s.raw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0){
        std::cout << "Connection error" << std::endl;
        return ;
    }
    std::cout << "Connected" << std::endl;

    //Receive messages indefinitely
    char buffer[BUFFER_SIZE+1];
    for (int i = 0; i != BUFFER_SIZE; ++i)
        buffer[i] = '\0';
    while (true) {
        int recvSize = recv(s.raw(), buffer, 100, 0);
        if (recvSize != SOCKET_ERROR){
            std::cout << "Received message: \"" << std::string(buffer) << "\"" << std::endl;
            _messages.push(std::string(buffer));
        }
    }
}

void Client::run(){
    socket.sendCommand("Test");

    if (!window || !image)
        return;

    draw();

    SDL_Delay(5000);
}

void Client::draw(){
    SDL_BlitSurface(image, 0, screen, 0);
    SDL_UpdateWindowSurface(window);
}