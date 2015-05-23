#include "Socket.h"

//TODO exceptions

const int Socket::MAX_CLIENTS = 10;
const int Socket::BUFFER_SIZE = 100;
int Socket::sockAddrSize = sizeof(sockaddr_in);



Socket::Socket(){
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

Socket::~Socket(){
    closesocket(s);
    WSACleanup();
}

int Socket::runServer(){
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

    listen(s, 3);

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
        FD_SET(s, &readFDs);
        for (int i = 0; i != MAX_CLIENTS; ++i){
            if (clientSocket[i] != 0)
                FD_SET(clientSocket[i], &readFDs);
        }
        
        // Poll for activity
        int activity = select(0, &readFDs, 0, 0, 0);
        if (activity == SOCKET_ERROR) {
            std::cout << "Error polling sockets: " << WSAGetLastError() << std::endl;
            return 1;
        }

        // Activity on server socket: new connection
        if (FD_ISSET(s, &readFDs)) {
            int i;
            for (i = 0; i != MAX_CLIENTS; ++i)
                if (clientSocket[i] == 0)
                    break;
            if (i == MAX_CLIENTS)
                std::cout << "No room for additional clients; all slots full" << std::endl;
            else {
                sockaddr_in clientAddr;
                clientSocket[i] = accept(s, (sockaddr*)&clientAddr, (int*)&sockAddrSize);
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
                getpeername(clientSocket[i], (sockaddr*)&clientAddr, (int*)&sockAddrSize);
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
    return 0;
}

int Socket::runClient(){
    // Server details
    sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);

    // Connect to server
    if (connect(s, (sockaddr*)&serverAddr, sockAddrSize) < 0){
        std::cout << "Connection error" << std::endl;
        return 1;
    }
    std::cout << "Connected" << std::endl;

    return 0;
}

// Send a client command to the server
void Socket::sendCommand(std::string msg) {
    if (send(s, msg.c_str(), msg.length(), 0) < 0)
        std::cout << "Failed to send command: " << msg << std::endl;
    else
        std::cout << "Sent command: " << msg << std::endl;
}
