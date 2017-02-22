#include "Client.h"

extern Renderer renderer;

static bool tryToConnect = false;
static void login(void *);

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

    drawLoginScreen();

    SDL_Delay(5);
}

void Client::drawLoginScreen() const{
    renderer.setDrawColor();
    renderer.clear();

    // Background
    _loginBack.draw();
    _loginFront.draw(_loginFrontOffset);

    // UI
    for (Element *element : _loginUI)
        element->draw();

    // Cursor
    _currentCursor->draw(_mouse);

    renderer.present();
}

void login(void *){
    tryToConnect = true;
}

void Client::initLoginScreen(){
    static const px_t
        BUTTON_W = 150,
        BUTTON_HEIGHT = 20,
        BUTTON_X = (SCREEN_X - BUTTON_W) / 2,
        BUTTON_Y = (SCREEN_Y - BUTTON_HEIGHT) / 2;

    Button *loginButton = new Button(Rect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_HEIGHT),
                                     "Connect", login);
    _loginUI.push_back(loginButton);
    
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

        case SDL_KEYDOWN:
            // Regular key input

            switch(e.key.keysym.sym) {

            case SDLK_ESCAPE:
                _loop = false;
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                break;
            }
        break;

        case SDL_MOUSEMOTION: {
            px_t x, y;
            SDL_GetMouseState(&x, &y);
            _mouse.x = x * SCREEN_X / static_cast<double>(renderer.width());
            _mouse.y = y * SCREEN_Y / static_cast<double>(renderer.height());
            _mouseMoved = true;
                
            Element::resetTooltip();
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
