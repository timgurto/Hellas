#include "Client.h"
#include "Particle.h"
#include "ui/Label.h"
#include "ui/TextBox.h"

extern Renderer renderer;
extern Args cmdLineArgs;

static bool tryToConnect = false;

static TextBox *nameBox;

static void quit(void*);

void Client::loginScreenLoop(){
    const double delta = _timeElapsed / 1000.0; // Fraction of a second that has elapsed
    _timeSinceConnectAttempt += _timeElapsed;

    // Deal with any messages from the server
    if (!_messages.empty()){
        handleMessage(_messages.front());
        _messages.pop();
    }

    if (tryToConnect)
        checkSocket();

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
    static const ParticleProfile *profile = findParticleProfile("loginFire");
    if (profile == nullptr)
        return;
    static const Point
        LEFT_BRAZIER(153, 281),
        RIGHT_BRAZIER(480, 282);
    size_t qty = profile->numParticlesContinuous(delta);
    for (size_t i = 0; i != qty; ++i) {
        Particle *p = profile->instantiate(LEFT_BRAZIER);
        _loginParticles.push_back(profile->instantiate(LEFT_BRAZIER));
        _loginParticles.push_back(profile->instantiate(RIGHT_BRAZIER));
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
    _loginFront.draw(_loginFrontOffset);

    // UI
    for (Element *element : _loginUI)
        element->draw();

    // Cursor
    _currentCursor->draw(_mouse);

    renderer.present();
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
    nameBox->text(username);
    _instance->_username = username;
    tryToConnect = true;
    SDL_StopTextInput();
}

void exit(void *data){
    bool &loop = *reinterpret_cast<bool *>(data);
    loop = false;
}

void Client::initLoginScreen(){
    // UI elements
    static const px_t
        BUTTON_W = 100,
        BUTTON_HEIGHT = 20,
        SCREEN_MID_X = 335,
        BUTTON_X = SCREEN_MID_X - BUTTON_W / 2,
        GAP = 20;
    px_t
        Y = (SCREEN_Y - BUTTON_HEIGHT) / 2;

    _loginUI.push_back(new Label(Rect(BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT),
                                 "Name:", Element::CENTER_JUSTIFIED));
    Y += Element::TEXT_HEIGHT;

    nameBox = new TextBox(Rect(BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT));
    nameBox->text(_username);
    TextBox::focus(nameBox);
    _loginUI.push_back(nameBox);

    SDL_StartTextInput();

    Y += Element::TEXT_HEIGHT + GAP;

    _loginUI.push_back(new Button(Rect(BUTTON_X, Y, BUTTON_W, BUTTON_HEIGHT), "Login", login));

    _loginUI.push_back(new Button(Rect(SCREEN_X - BUTTON_W - GAP, SCREEN_Y - BUTTON_HEIGHT - GAP,
                                       BUTTON_W, BUTTON_HEIGHT), "Quit", exit, &_loop));
    
    std::string serverIP;
    if (cmdLineArgs.contains("server-ip"))
        serverIP = cmdLineArgs.getString("server-ip");
    else{
        serverIP = _defaultServerAddress;
    }
    _loginUI.push_back(new Label(Rect(GAP, SCREEN_Y -  Element::TEXT_HEIGHT - GAP,
                                      200, Element::TEXT_HEIGHT),
                                 "Server: " + serverIP));

    // Images
    _loginFront = Texture(std::string("Images/loginFront.png"), Color::MAGENTA);
    _loginBack = Texture(std::string("Images/loginBack.png"));
}

void Client::cleanUpLoginScreen(){
    for (Element *element : _loginUI)
        delete element;
}

void Client::handleLoginInput(double delta){
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
            _mouse.x = x * SCREEN_X / static_cast<double>(renderer.width());
            _mouse.y = y * SCREEN_Y / static_cast<double>(renderer.height());
                
            for (Element *element : _loginUI)
                if (element->visible())
                    element->onMouseMove(_mouse);

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect()))
                        element->onLeftMouseDown(_mouse);
                break;

            case SDL_BUTTON_RIGHT:
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect()))
                        element->onRightMouseDown(_mouse);
                break;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                for (Element *element : _loginUI)
                    if (element->visible() && collision(_mouse, element->rect())) {
                        element->onLeftMouseUp(_mouse);
                        break;
                    }
                break;

            case SDL_BUTTON_RIGHT:
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
