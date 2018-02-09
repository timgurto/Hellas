#include <algorithm>

#include "Client.h"
#include "Particle.h"
#include "../versionUtil.h"
#include "ui/Indicator.h"
#include "ui/Label.h"
#include "ui/TextBox.h"

extern Renderer renderer;
extern Args cmdLineArgs;

static TextBox *nameBox;

void Client::loginScreenLoop(){
    const double delta = _timeElapsed / 1000.0; // Fraction of a second that has elapsed
    _timeSinceConnectAttempt += _timeElapsed;

    connectToServer();

    // Deal with any messages from the server
    if (!_messages.empty()){
        handleMessage(_messages.front());
        _messages.pop();
    }

    handleLoginInput(delta);

    updateLoginParticles(delta);

    drawLoginScreen();

    SDL_Delay(5);
}

void Client::updateLoginParticles(double delta){
    // Update existing particles
    for (auto it = _loginParticles.begin(); it != _loginParticles.end();){
        (*it)->update(delta);
        auto nextIterator = it; ++nextIterator;
        if ((*it)->markedForRemoval())
            _loginParticles.erase(it);
        it = nextIterator;
    }

    // Add new particles
    std::vector<const ParticleProfile *> profiles;
    profiles.push_back(findParticleProfile("loginFireLarge"));
    profiles.push_back(findParticleProfile("loginFire"));
    const MapPoint
        LEFT_BRAZIER(153, 281),
        RIGHT_BRAZIER(480, 282);

    for (const ParticleProfile *profile : profiles){
        if (profile == nullptr)
            continue;
        size_t qty = profile->numParticlesContinuous(delta);
        for (size_t i = 0; i != qty; ++i) {
            Particle *p = profile->instantiate(LEFT_BRAZIER);
            _loginParticles.push_back(profile->instantiate(LEFT_BRAZIER));
            _loginParticles.push_back(profile->instantiate(RIGHT_BRAZIER));
        }
    }
}

void Client::drawLoginScreen() const{
    renderer.setDrawColor();
    renderer.clear();

    // Background
    _loginBack.draw();

    // Particles
    for (auto particle : _loginParticles)
        particle->draw(*this);

    // Braziers
    _loginFront.draw(_config.loginFrontOffset);

    // UI
    for (Element *element : _loginUI)
        element->draw();

    // Windows
    for (windows_t::const_reverse_iterator it = _windows.rbegin(); it != _windows.rend(); ++it)
        (*it)->draw();

    // Cursor
    _currentCursor->draw(_mouse);

    renderer.present();
}

void Client::connectToServer() {
        if (_invalidUsername)
            return;

    // Ensure connected to server
    if (_connectionStatus != CONNECTED && _timeSinceConnectAttempt >= CONNECT_RETRY_DELAY) {
        _timeSinceConnectAttempt = 0;
        // Server details
        std::string serverIP;
        if (cmdLineArgs.contains("server-ip"))
            serverIP = cmdLineArgs.getString("server-ip");
        else {
            serverIP = _defaultServerAddress;
        }
        sockaddr_in serverAddr;
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
        serverAddr.sin_family = AF_INET;

        // Select server port
#ifdef _DEBUG
        auto port = DEBUG_PORT;
#else
        auto port = PRODUCTION_PORT;
#endif
        if (cmdLineArgs.contains("server-port"))
            port = cmdLineArgs.getInt("server-port");
        serverAddr.sin_port = htons(port);

        if (connect(_socket.getRaw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0) {
            showErrorMessage("Connection error: "s + toString(WSAGetLastError()), Color::FAILURE);
            _connectionStatus = CONNECTION_ERROR;
            _serverConnectionIndicator->set(Indicator::FAILED);
        } else {
            _serverConnectionIndicator->set(Indicator::SUCCEEDED);
            sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
            _connectionStatus = CONNECTED;
        }
    }

    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = { 0, 10000 };
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        showErrorMessage("Error polling sockets: "s + toString(WSAGetLastError()), Color::FAILURE);
        _serverConnectionIndicator->set(Indicator::FAILED);
        return;
    }
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        static char buffer[BUFFER_SIZE + 1];
        int charsRead = recv(_socket.getRaw(), buffer, BUFFER_SIZE, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0) {
            buffer[charsRead] = '\0';
            _messages.push(std::string(buffer));
        }
    }

}

void Client::login(void *){
    for (char c : nameBox->text()){
        if ((c < 'A' || c > 'Z') &&
            (c < 'a' || c > 'z')){
            nameBox->text("Letters only, please");
            return;
        }
    }
    if (nameBox->text().empty()){
        nameBox->text("At least 1 letter, please");
        return;
    }
    std::string username = nameBox->text();
    std::transform(username.begin(), username.end(), username.begin(), tolower);
    nameBox->text(username);
    SDL_StopTextInput();

    _instance->_username = username;
    _instance->sendMessage(CL_LOGIN_EXISTING, makeArgs(username, version()));
}

void exit(void *data){
    bool &loop = *reinterpret_cast<bool *>(data);
    loop = false;
}

void Client::initLoginScreen(){
    // UI elements
    const px_t
        BUTTON_W = 100,
        BUTTON_HEIGHT = 20,
        SCREEN_MID_X = 335,
        BUTTON_X = SCREEN_MID_X - BUTTON_W / 2,
        GAP = 20,
        LEFT_MARGIN = 10,
        LABEL_GAP = Element::TEXT_HEIGHT + 2;
    px_t
        Y = (SCREEN_Y - BUTTON_HEIGHT) / 2;

    _loginUI.push_back(new OutlinedLabel({ BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT+5 },
                                 "Name:", Element::CENTER_JUSTIFIED));
    Y += Element::TEXT_HEIGHT+1;

    nameBox = new TextBox({ BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT });
    nameBox->text(_username);
    TextBox::focus(nameBox);
    _loginUI.push_back(nameBox);

    SDL_StartTextInput();

    Y += Element::TEXT_HEIGHT + GAP;

    _loginUI.push_back(new Button({ BUTTON_X, Y, BUTTON_W, BUTTON_HEIGHT }, "Login", login));

    _loginUI.push_back(new Button({ SCREEN_X - BUTTON_W - GAP, SCREEN_Y - BUTTON_HEIGHT - GAP,
                                       BUTTON_W, BUTTON_HEIGHT }, "Quit", exit, &_loop));

    {
        Y = 318;

        // Server-connection indicator
        const px_t
            INDICATOR_LABEL_X = LEFT_MARGIN + 15,
            INDICATOR_Y_OFFSET = -1;
        _serverConnectionIndicator = new Indicator({ LEFT_MARGIN, Y - INDICATOR_Y_OFFSET });
        _loginUI.push_back(_serverConnectionIndicator);
        _loginUI.push_back(new OutlinedLabel({ INDICATOR_LABEL_X, Y, 100 , Element::TEXT_HEIGHT + 5 },
            "Server connection"s));
        Y += LABEL_GAP;

        // Server IP
        std::string serverIP;
        if (cmdLineArgs.contains("server-ip"))
            serverIP = cmdLineArgs.getString("server-ip");
        else {
            serverIP = _defaultServerAddress;
        }
        _loginUI.push_back(new OutlinedLabel({ LEFT_MARGIN, Y, 200, Element::TEXT_HEIGHT + 5 },
            "Server: " + serverIP));
        Y += LABEL_GAP;

        // Client version
        _loginUI.push_back(new OutlinedLabel({ LEFT_MARGIN, Y, 200, Element::TEXT_HEIGHT + 5 },
            "Client version: " + version()));
        Y += LABEL_GAP;
    }


    // Images
    _loginFront = Texture(std::string("Images/loginFront.png"), Color::MAGENTA);
    _loginBack = Texture(std::string("Images/loginBack.png"));

    if (cmdLineArgs.contains("auto-login"))
        login(nullptr);
}

void Client::cleanUpLoginScreen(){
    for (Element *element : _loginUI)
        delete element;
}

void Client::handleLoginInput(double delta){
    auto mouseEventWasOnWindow = false;
    static SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        std::ostringstream oss;
        switch(e.type) {
        case SDL_QUIT:
            _loop = false;
            break;

        case SDL_TEXTINPUT:
            TextBox::addText(e.text.text);
            break;

        case SDL_KEYDOWN:
            switch(e.key.keysym.sym) {

            case SDLK_ESCAPE:
                _loop = false;
                break;

            case SDLK_BACKSPACE:
                TextBox::backspace();
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                login(nullptr);
                break;
            }
            break;

        case SDL_MOUSEMOTION: {
            px_t x, y;
            SDL_GetMouseState(&x, &y);
            _mouse.x = toInt(x * SCREEN_X / static_cast<double>(renderer.width()));
            _mouse.y = toInt(y * SCREEN_Y / static_cast<double>(renderer.height()));
                
            for (Element *element : _loginUI)
                if (element->visible())
                    element->onMouseMove(_mouse);
            for (Window *window : _windows)
                if (window->visible())
                    window->onMouseMove(_mouse);

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                for (Window *window : _windows)
                    if (window->visible() && collision(_mouse, window->rect())) {
                        window->onLeftMouseDown(_mouse);
                        mouseEventWasOnWindow = true;
                        break;
                    }
                if (mouseEventWasOnWindow)
                    break;
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect())) {
                        element->onLeftMouseDown(_mouse);
                        mouseEventWasOnWindow = true;
                    }
                break;

            case SDL_BUTTON_RIGHT:
                for (Window *window : _windows)
                    if (window->visible() && collision(_mouse, window->rect())) {
                        window->onRightMouseDown(_mouse);
                        mouseEventWasOnWindow = true;
                        break;
                    }
                if (mouseEventWasOnWindow)
                    break;
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect())){
                        element->onRightMouseDown(_mouse);
                        mouseEventWasOnWindow = true;
                    }
                break;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                for (Window *window : _windows)
                    if (window->visible() && collision(_mouse, window->rect())) {
                        window->onLeftMouseUp(_mouse);
                        mouseEventWasOnWindow = true;
                        break;
                    }
                if (mouseEventWasOnWindow)
                    break;
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect())) {
                        element->onLeftMouseUp(_mouse);
                        mouseEventWasOnWindow = true;
                        break;
                    }
                break;

            case SDL_BUTTON_RIGHT:
                for (Window *window : _windows)
                    if (window->visible() && collision(_mouse, window->rect())) {
                        window->onRightMouseUp(_mouse);
                        mouseEventWasOnWindow = true;
                        break;
                    }
                if (mouseEventWasOnWindow)
                    break;
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect())) {
                        element->onRightMouseUp(_mouse);
                        break;
                    }
                break;
            }
            break;

        case SDL_WINDOWEVENT:
            switch(e.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                renderer.updateSize();
                for (Element *element : _loginUI)
                    element->forceRefresh();
                break;
            }

        default:
            // Unhandled event
            ;
        }
    }
}
