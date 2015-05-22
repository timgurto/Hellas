#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

const int MAX_CLIENTS = 10;

int server();

int client();
void sendCommand(std::string msg);

#endif