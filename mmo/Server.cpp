#include <windows.h>
#include <cassert>
#include <SDL.h>

#include "Socket.h"
#include "Server.h"

const int Server::MAX_CLIENTS = 10;
const int Server::BUFFER_SIZE = 100;

DWORD WINAPI startSocketServer(LPVOID server){
    ((Server*)server)->runSocketServer();
    return 0;
}

void Server::runSocketServer(){
    Socket s;
    // Socket details
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    s.bind(serverAddr);
    s.listen();

    fd_set readFDs;
    SOCKET clientSocket[MAX_CLIENTS];
    for (int i = 0; i != MAX_CLIENTS; ++i)
        clientSocket[i] = 0;
    char buffer[BUFFER_SIZE+1];
    for (int i = 0; i != BUFFER_SIZE; ++i)
        buffer[i] = '\0';

    while (true) {
        // Populate socket list with active sockets
        FD_ZERO(&readFDs);
        FD_SET(s.raw(), &readFDs);
        for (int i = 0; i != MAX_CLIENTS; ++i){
            if (clientSocket[i] != 0)
                FD_SET(clientSocket[i], &readFDs);
        }
        
        // Poll for activity
        int activity = select(0, &readFDs, 0, 0, 0);
        if (activity == SOCKET_ERROR) {
            std::cout << "Error polling sockets: " << WSAGetLastError() << std::endl;
            return;
        }

        // Activity on server socket: new connection
        if (FD_ISSET(s.raw(), &readFDs)) {
            int i;
            for (i = 0; i != MAX_CLIENTS; ++i)
                if (clientSocket[i] == 0)
                    break;
            if (i == MAX_CLIENTS)
                std::cout << "No room for additional clients; all slots full" << std::endl;
            else {
                sockaddr_in clientAddr;
                clientSocket[i] = accept(s.raw(), (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
                if (clientSocket[i] == SOCKET_ERROR) {
                    std::cout << "Error accepting connection: " << WSAGetLastError() << std::endl;
                    clientSocket[i] = 0;
                } else {
                    std::cout << "Connection accepted: "
                              << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl
                              << "ID= " << i << ", socket number = " << clientSocket[i] << std::endl;
                }
            }
        }

        // Activity on client socket: message received or client disconnected
        for (int i = 0; i != MAX_CLIENTS; ++i) {
            if (FD_ISSET(clientSocket[i], &readFDs)) {
                sockaddr_in clientAddr;
                getpeername(clientSocket[i], (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
                int charsRead = recv(clientSocket[i], buffer, BUFFER_SIZE, 0);
                if (charsRead == SOCKET_ERROR) {
                    int err = WSAGetLastError();
                    if (err == WSAECONNRESET) {
                        // Client disconnected
                        closesocket(clientSocket[i]);
                        clientSocket[i] = 0;
                        std::cout << "Client " << i << " disconnected" << std::endl;
                    } else {
                        std::cout << "Error receiving message: " << err << std::endl;
                    }
                } else if (charsRead == 0) {
                    // Client disconnected
                    closesocket(clientSocket[i]);
                    clientSocket[i] = 0;
                    std::cout << "Client " << i << " disconnected" << std::endl;
                } else {
                    // Message received
                    buffer[charsRead] = '\0';
                    std::cout << "Message received from client " << i << ": " << buffer << std::endl;
                }
            }
        }

    }

    // Should never reach here
    assert (false);
}

Server::Server():
window(0){
    DWORD socketThreadID;
    CreateThread(0, 0, &startSocketServer, this, 0, &socketThreadID);
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

    SDL_Delay(10000);
}
