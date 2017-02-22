#include "Client.h"

extern Renderer renderer;

void Client::loginScreenLoop(){
    _timeSinceConnectAttempt += _timeElapsed;

    // Deal with any messages from the server
    if (!_messages.empty()){
        handleMessage(_messages.front());
        _messages.pop();
    }

    checkSocket();

    // Draw
    renderer.setDrawColor();
    renderer.clear();
    Texture(std::string("Images/login.png")).draw();
    renderer.present();

    SDL_Delay(5);
}
