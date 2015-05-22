#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int server();

int client();

#endif