#include <windows.h>
#include <SDL.h>
#include <string>
#include <sstream>

#include "Client.h"
#include "messageCodes.h"

const int Client::BUFFER_SIZE = 100;

DWORD WINAPI startSocketClient(LPVOID client){
    ((Client*)client)->runSocketClient();
    return 0;
}

Client::Client():
window(0),
image(0),
screen(0),
_location(std::make_pair(0, 0)),
_loop(true){
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
    // Server details
    sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);

    // Connect to server
    if (connect(_socket.raw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0){
        std::cout << "Connection error" << std::endl;
        return ;
    }
    std::cout << "Connected" << std::endl;

    //Receive messages indefinitely
    char buffer[BUFFER_SIZE+1];
    for (int i = 0; i != BUFFER_SIZE; ++i)
        buffer[i] = '\0';
    while (true) {
        int charsRead = recv(_socket.raw(), buffer, 100, 0);
        if (charsRead != SOCKET_ERROR){
            buffer[charsRead] = '\0';
            std::cout << "Received message: \"" << std::string(buffer) << "\"" << std::endl;
            _messages.push(std::string(buffer));
        }
    }
}

void Client::run(){

    if (!window || !image)
        return;

    while (_loop) {
        // Deal with any messages from the server
        if (!_messages.empty()){
            handleMessage(_messages.front());
            _messages.pop();
        }

        // Handle user events
        static SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            std::ostringstream oss;
            switch(e.type) {
            case SDL_QUIT:
                _loop = false;
                break;

            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_UP:
                    oss << '[' << REQ_MOVE_UP << ']';
                    _socket.sendCommand(oss.str());
                    break;
                case SDLK_DOWN:
                    oss << '[' << REQ_MOVE_DOWN << ']';
                    _socket.sendCommand(oss.str());
                    break;
                case SDLK_LEFT:
                    oss << '[' << REQ_MOVE_LEFT << ']';
                    _socket.sendCommand(oss.str());
                    break;
                case SDLK_RIGHT:
                    oss << '[' << REQ_MOVE_RIGHT << ']';
                    _socket.sendCommand(oss.str());
                    break;
                }
                break;

            default:
                //Unhandled event
                ;
            }
        }

        // Draw
        draw();
    }
}

void Client::draw(){
    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 96, 0));
    SDL_Rect drawLoc;
    drawLoc.x = _location.first;
    drawLoc.y = _location.second;
    SDL_BlitSurface(image, 0, screen, &drawLoc);
    SDL_UpdateWindowSurface(window);
}

void Client::handleMessage(std::string msg){
    std::istringstream iss(msg);
    int msgCode;
    char del;
    while (iss.peek() == '[') {
        iss >>del >> msgCode >> del;
        switch(msgCode) {

        case MSG_LOCATION:
            int x, y;
            iss >> x >> del >> y >> del;
            if (del != ']')
                return;
            _location = std::make_pair(x, y);

        default:
            ;
        }
    }
}
