#include "socket.h"

//TODO use class
//TODO exceptions

static SOCKET s;

void init(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        return;
    }
    std::cout << "Winsock initialized" << std::endl;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cout << "Could not create socket: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "Socket created" << std::endl;
}

int server(){
    init();

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

    //Listen for a connection
    listen(s, 3);
    std::cout << "Waiting for connections" << std::endl;
    sockaddr_in client;
    static int c = sizeof(sockaddr_in);
    SOCKET newSocket = accept(s, (sockaddr*)&client, &c);
    if (newSocket == INVALID_SOCKET)
        std::cout << "Error accepting connection: " << WSAGetLastError() << std::endl;
    else{
        std::cout << "Connection accepted: "
                  << inet_ntoa(client.sin_addr)
                  << ":"
                  << ntohs(client.sin_port)
                  << std::endl;
        static char buffer[101];
        while (true) {
            int recvSize = recv(newSocket, buffer, 100, 0);
            if (recvSize != SOCKET_ERROR)
                std::cout << "Received message: \"" << std::string(buffer) << "\"" << std::endl;
        }
    }

    //Should never reach here
    closesocket(s);
    return 0;
}

int client(){
    init();

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

    return 0;
}

// Send a client command to the server
void sendCommand(std::string msg) {
    if (send(s, msg.c_str(), msg.length(), 0) < 0)
        std::cout << "Failed to send command: " << msg << std::endl;
    else
        std::cout << "Sent command: " << msg << std::endl;
}