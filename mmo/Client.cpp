#include <windows.h>

#include "Client.h"

Client::Client(){
    socket.runClient();
}

void Client::run(){
    socket.sendCommand("Test");
    while (true)
        ;
}
