#include <algorithm>
#include <thread>

#include "Client.h"
#include "Particle.h"
#include "../versionUtil.h"
#include "ui/Indicator.h"
#include "ui/Label.h"
#include "ui/TextBox.h"
#include "ui/Window.h"

extern Renderer renderer;
extern Args cmdLineArgs;

static TextBox *nameBox{ nullptr };
static TextBox *newNameBox{ nullptr };
static Button *loginButton{ nullptr };
static Button *createButton{ nullptr };
static ChoiceList *classList{ nullptr };
static List *classDescription{ nullptr };
static OutlinedLabel *loginErrorLabel{ nullptr };

static void showError(const std::string &message, const Color &color) {
    loginErrorLabel->changeText(message);
    loginErrorLabel->setColor(color);
}

void Client::loginScreenLoop(){
    const double delta = _timeElapsed / 1000.0; // Fraction of a second that has elapsed
    _timeSinceConnectAttempt += _timeElapsed;

    if (!_threadIsConnectingToServer) {
        _threadIsConnectingToServer = true;
        std::thread{ connectToServerStatic }.detach();
    }

    // Send ping
    if (_time - _lastPingSent > PING_FREQUENCY) {
        sendMessage(CL_PING, makeArgs(_time));
        _lastPingSent = _time;
    }

    // Ensure server connectivity
    if (_time - _lastPingReply > SERVER_TIMEOUT) {
        _serverConnectionIndicator->set(Indicator::FAILED);
        _connection.clearSocket();
        _loggedIn = false;
    }

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

    // Tooltip
    drawTooltip();

    // Cursor
    _currentCursor->draw(_mouse);

    renderer.present();
}

void Client::connectToServerStatic() {
    auto &client = *_instance;
    client.connectToServer();
}

void Client::connectToServer() {
    // Ensure connected to server
    if (_connection.state() != Connection::CONNECTED && _timeSinceConnectAttempt >= CONNECT_RETRY_DELAY) {
        _connection.state(Connection::TRYING_TO_CONNECT);
        _timeSinceConnectAttempt = 0;

        if (_serverConnectionIndicator)
            _serverConnectionIndicator->set(Indicator::IN_PROGRESS);

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

        if (connect(_connection.socket().getRaw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0) {
            showError("Connection error: "s + toString(WSAGetLastError()), Color::FAILURE);
            _connection.state(Connection::CONNECTION_ERROR);
            _serverConnectionIndicator->set(Indicator::FAILED);
        } else {
            _serverConnectionIndicator->set(Indicator::SUCCEEDED);
            sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
            _connection.state(Connection::CONNECTED);
        }
    }

    updateCreateButton(nullptr);
    updateLoginButton(nullptr);

    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_connection.socket().getRaw(), &readFDs);
    static timeval selectTimeout = { 0, 10000 };
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        showError("Error polling sockets: "s + toString(WSAGetLastError()), Color::FAILURE);
        _serverConnectionIndicator->set(Indicator::FAILED);
    } else if (FD_ISSET(_connection.socket().getRaw(), &readFDs)) {
        const auto BUFFER_SIZE = 1023;
        static char buffer[BUFFER_SIZE + 1];
        int charsRead = recv(_connection.socket().getRaw(), buffer, BUFFER_SIZE, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0) {
            buffer[charsRead] = '\0';
            _messages.push(std::string(buffer));
        }
    }

    _threadIsConnectingToServer = false;
}

void Client::updateLoginButton(void *) {
    loginButton->clearTooltip();
    loginButton->disable();
    nameBox->forcePascalCase();

    if (Client::_instance->_connection.state() != Connection::CONNECTED)
        ;// loginButton->setTooltip("Not connected to server");

    else if (!isUsernameValid(nameBox->text()))
        ;// loginButton->setTooltip("Please enter a valid username");

    else
        loginButton->enable();
}

void Client::updateCreateButton(void *) {
    createButton->clearTooltip();
    createButton->disable();
    newNameBox->forcePascalCase();

    if (Client::_instance->_connection.state() != Connection::CONNECTED)
        ;// createButton->setTooltip("Not connected to server");

    else if (!isUsernameValid(newNameBox->text()))
        ;// createButton->setTooltip("Please enter a valid username");

    else if (classList->getSelected().empty())
        ;// createButton->setTooltip("Please choose a class");

    else
        createButton->enable();
}

void Client::updateClassDescription() {
    updateCreateButton(nullptr);

    classDescription->clearChildren();
    auto classID = classList->getSelected();
    if (classID.empty())
        return;

    const auto &description = _instance->_classes[classID].description();
    static auto wrapper = WordWrapper{ _instance->defaultFont(), classDescription->contentWidth() };
    auto lines = wrapper.wrap(description);

    for (const auto &line : lines)
        classDescription->addChild(new Label({}, line));
}

void Client::initCreateWindow() {

    const auto
        L_PANE_W = 100_px,
        R_PANE_W = 200_px,
        MID_PANE = 40_px,
        PANE_H = 120_px,
        MARGIN = 10_px,
        BUTTON_HEIGHT = 20,
        BUTTON_WIDTH = 100,
        WIN_W = L_PANE_W + R_PANE_W + 3 * MARGIN,
        WIN_H = PANE_H + 3 * MARGIN + BUTTON_HEIGHT,
        WIN_X = (SCREEN_X - WIN_W) / 2,
        WIN_Y = (SCREEN_Y - WIN_H) / 2,
        GAP = 2;

    _createWindow = Window::WithRectAndTitle({ WIN_X, WIN_Y, WIN_W, WIN_H }, "Create Account"s);
    addWindow(_createWindow);

    auto infoPane = new Element({ L_PANE_W + 2 * MARGIN, MARGIN, R_PANE_W, PANE_H });
    _createWindow->addChild(infoPane);

    {
        auto inputPane = new Element({ MARGIN, MARGIN, L_PANE_W, PANE_H });
        _createWindow->addChild(inputPane);
        auto y = 0_px;

        inputPane->addChild(new Label({ 0, y, 100, Element::TEXT_HEIGHT }, "Name:"s));
        newNameBox = new TextBox({ MID_PANE, y, L_PANE_W - MID_PANE, 0 },
            TextBox::LETTERS);
        newNameBox->setOnChange(updateCreateButton);
        inputPane->addChild(newNameBox);
        infoPane->addChild(new Label({ 0, y, R_PANE_W, Element::TEXT_HEIGHT },
            "(Names must contain 3-20 characters)"s));
        y += newNameBox->height() + GAP;

        inputPane->addChild(new Label({ 0, y, 100, Element::TEXT_HEIGHT }, "Class:"s));
        const auto CLASS_LIST_HEIGHT = 50_px;
        classList = new ChoiceList({ MID_PANE, y, L_PANE_W - MID_PANE, CLASS_LIST_HEIGHT },
            Element::TEXT_HEIGHT);
        inputPane->addChild(classList);
        for (const auto &pair : _classes) {
            auto label = new Label({}, " "s + pair.second.name());
            label->id(pair.first);
            classList->addChild(label);
        }

        classList->onSelect = updateClassDescription;
        classDescription = new List({ 0, y, R_PANE_W, PANE_H - y });
        infoPane->addChild(classDescription);
    }

    const auto
        BUTTON_X = (WIN_H - BUTTON_WIDTH) / 2,
        BUTTON_Y = WIN_H - BUTTON_HEIGHT - MARGIN;
    createButton = new Button({ BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT },
        "Create account"s, createAccount);
    _createWindow->addChild(createButton);
}

void Client::createAccount(void *) {
    auto username = newNameBox->text();
    newNameBox->text(username);

    _instance->_username = username;
    const auto &selectedClass = classList->getSelected();
    _instance->sendMessage(CL_LOGIN_NEW, makeArgs(username, selectedClass, version()));
}

void Client::login(void *) {
    if (_instance->_username.empty()) {
        _instance->_username = nameBox->text();
    }

    _instance->sendMessage(CL_LOGIN_EXISTING, makeArgs(_instance->_username, version()));
}

void exit(void *data){
    bool &loop = *reinterpret_cast<bool *>(data);
    loop = false;
}

void Client::initLoginScreen(){
    initCreateWindow();

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

    _loginUI.push_back(new OutlinedLabel({ BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT + 5 },
        "Name:", Element::CENTER_JUSTIFIED));
    Y += Element::TEXT_HEIGHT + 1;

    nameBox = new TextBox({ BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT }, TextBox::LETTERS);
    nameBox->text(_username);
    TextBox::focus(nameBox);
    nameBox->setOnChange(updateLoginButton);
    _loginUI.push_back(nameBox);

    SDL_StartTextInput();

    Y += Element::TEXT_HEIGHT + GAP;

    loginButton = new Button({ BUTTON_X, Y, BUTTON_W, BUTTON_HEIGHT }, "Login", login);
    _loginUI.push_back(loginButton);
    updateLoginButton(nullptr);

    // Left-hand content
    {
        auto
            y = 318_px;

        // Server-connection indicator
        const px_t
            INDICATOR_LABEL_X = LEFT_MARGIN + 15,
            INDICATOR_Y_OFFSET = -1;
        _serverConnectionIndicator = new Indicator({ LEFT_MARGIN, y - INDICATOR_Y_OFFSET });
        _loginUI.push_back(_serverConnectionIndicator);
        auto label = new OutlinedLabel({ INDICATOR_LABEL_X, y, 200 , Element::TEXT_HEIGHT + 5 },
            "Connection with server"s);
        label->setColor(Color::ITEM_STATS);
        _loginUI.push_back(label);
        y += LABEL_GAP;

        // Server IP
        std::string serverIP;
        if (cmdLineArgs.contains("server-ip"))
            serverIP = cmdLineArgs.getString("server-ip");
        else {
            serverIP = _defaultServerAddress;
        }
        label = new OutlinedLabel({ LEFT_MARGIN, y, 200, Element::TEXT_HEIGHT + 5 },
            "Server: " + serverIP);
        label->setColor(Color::ITEM_STATS);
        _loginUI.push_back(label);
        y += LABEL_GAP;

        // Client version
        label = new OutlinedLabel({ LEFT_MARGIN, y, 200, Element::TEXT_HEIGHT + 5 },
            "Client version: " + version());
        label->setColor(Color::ITEM_STATS);
        _loginUI.push_back(label);
        y += LABEL_GAP;
    }

    // Right-hand content
    {
        _loginUI.push_back(new Button({ SCREEN_X - BUTTON_W - GAP, SCREEN_Y - BUTTON_HEIGHT - GAP,
            BUTTON_W, BUTTON_HEIGHT }, "Quit", exit, &_loop));

        auto
            y = 250_px,
            BUTTON_X = SCREEN_X - BUTTON_W - GAP;

        auto createButton = new Button({ BUTTON_X, y, BUTTON_W, BUTTON_HEIGHT }, "Create new account",
            showWindowInFront, _createWindow);
        _loginUI.push_back(createButton);
        updateCreateButton(nullptr);
    }

    // Images
    _loginFront = Texture(std::string("Images/loginFront.png"), Color::MAGENTA);
    _loginBack = Texture(std::string("Images/loginBack.png"));

    if (cmdLineArgs.contains("auto-login"))
        login(nullptr);

    loginErrorLabel = new OutlinedLabel({ 0,0,200,15 }, {});
    _loginUI.push_back(loginErrorLabel);
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

            Element::resetTooltip();
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
