#include "socket.h"

int server(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        return 1;
    }
    std::cout << "Winsock initialized" << std::endl;

    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cout << "Could not create socket: " << WSAGetLastError() << std::endl;
        return 1;
    }
    std::cout << "Socket created" << std::endl;

    // Socket details
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    // Bind
    if (bind(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR){
        std::cout << "Error binding socket: " << WSAGetLastError() <<  std::endl;
        return 1;
    }
    std::cout << "Socket bound" << std::endl;

    while (true);

    //closesocket(s);
}

int client(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        return 1;
    }
    std::cout << "Winsock initialized" << std::endl;

    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cout << "Could not create socket: " << WSAGetLastError() << std::endl;
        return 1;
    }
    std::cout << "Socket created" << std::endl;

    // Server details
    sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    // Connect to server
    if (connect(s, (sockaddr*)&server, sizeof(server)) < 0){
        std::cout << "Connection error" << std::endl;
        return 1;
    }
    std::cout << "Connected" << std::endl;
}