#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        return 1;
    }
    std::cout << "Winsock initialized" << std::endl;

    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cout << "Could not create socket: error " << WSAGetLastError() << std::endl;
        return -1;
    }
    std::cout << "Socket created" << std::endl;
}