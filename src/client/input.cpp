// (C) 2016 Tim Gurto

#include "Client.h"
#include "Renderer.h"

extern Renderer renderer;

void Client::handleInput(double delta){
    static SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        std::ostringstream oss;
        switch(e.type) {
        case SDL_QUIT:
            _loop = false;
            break;

        case SDL_TEXTINPUT:
            if (_enteredText.size() < MAX_TEXT_ENTERED)
                _enteredText.append(e.text.text);
            break;

        case SDL_KEYDOWN:
            if (SDL_IsTextInputActive()) {
                // Text input

                switch(e.key.keysym.sym) {

                case SDLK_ESCAPE:
                    SDL_StopTextInput();
                    _enteredText = "";
                    break;

                case SDLK_BACKSPACE:
                    if (_enteredText.size() > 0) {
                        _enteredText.erase(_enteredText.size() - 1);
                    }
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    SDL_StopTextInput();
                    if (_enteredText != "") {
                        if (_enteredText.at(0) == '/') {
                            // Perform command
                            performCommand(_enteredText);
                        } else {
                            performCommand("/say " + _enteredText);
                        }
                        _enteredText = "";
                    }
                    break;
                }

            } else {
                // Regular key input

                switch(e.key.keysym.sym) {

                case SDLK_ESCAPE:
                {
                    Window *frontMostVisibleWindow = 0;
                    for (Window *window : _windows)
                        if (window->visible()){
                            frontMostVisibleWindow = window;
                            break;
                        }

                    if (_actionLength != 0)
                        sendMessage(CL_CANCEL_ACTION);
                    else if (frontMostVisibleWindow)
                        frontMostVisibleWindow->hide();
                    else
                        _loop = false;
                    break;
                }

                case SDLK_SLASH:
                    SDL_StartTextInput();
                    _enteredText = "/";
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    SDL_StartTextInput();
                    break;

                case SDLK_c:
                    _craftingWindow->toggleVisibility();
                    removeWindow(_craftingWindow);
                    addWindow(_craftingWindow);
                    break;

                case SDLK_l:
                    _chatContainer->toggleVisibility();
                    break;

                case SDLK_i:
                    _inventoryWindow->toggleVisibility();
                    removeWindow(_inventoryWindow);
                    addWindow(_inventoryWindow);
                    break;

                case SDLK_r:
                    if (!_lastWhisperer.empty()) {
                        SDL_StartTextInput();
                        _enteredText = "/whisper " + _lastWhisperer + " ";
                    }
                    break;
                }
            }
            break;

        case SDL_MOUSEMOTION: {
            _mouse.x = e.motion.x * SCREEN_X / static_cast<double>(renderer.width());
            _mouse.y = e.motion.y * SCREEN_Y / static_cast<double>(renderer.height());
            _mouseMoved = true;
                
            Element::resetTooltip();
            for (Window *window : _windows)
                if (window->visible())
                    window->onMouseMove(_mouse);
            for (Element *element : _ui)
                if (element->visible())
                    element->onMouseMove(_mouse);

            if (!_loaded)
                break;

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                _leftMouseDown = true;

                // Send onLeftMouseDown to all visible windows
                for (Window *window : _windows)
                    if (window->visible())
                        window->onLeftMouseDown(_mouse);
                for (Element *element : _ui)
                    if (element->visible()&& collision(_mouse, element->rect()))
                        element->onLeftMouseDown(_mouse);

                // Bring top clicked window to front
                for (windows_t::iterator it = _windows.begin(); it != _windows.end(); ++it) {
                    Window &window = **it;
                    if (window.visible() && collision(_mouse, window.rect())) {
                        _windows.erase(it); // Invalidates iterator.
                        addWindow(&window);
                        break;
                    }
                }

                _leftMouseDownEntity = getEntityAtMouse();
                break;

            case SDL_BUTTON_RIGHT:
                // Send onRightMouseDown to all visible windows
                for (Window *window : _windows)
                    if (window->visible())
                        window->onRightMouseDown(_mouse);
                for (Element *element : _ui)
                    if (element->visible()&& collision(_mouse, element->rect()))
                        element->onRightMouseDown(_mouse);

                _rightMouseDownEntity = getEntityAtMouse();
                break;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (!_loaded)
                break;

            switch (e.button.button) {
            case SDL_BUTTON_LEFT: {
                _leftMouseDown = false;

                // Construct item
                if (Container::getUseItem()) {
                    int
                        x = toInt(_mouse.x - offset().x),
                        y = toInt(_mouse.y - offset().y);
                    sendMessage(CL_CONSTRUCT, makeArgs(Container::useSlot, x, y));
                    prepareAction(std::string("Constructing ") +
                                  _inventory[Container::useSlot].first->name());
                    break;
                }

                bool mouseUpOnWindow = false;
                for (Window *window : _windows)
                    if (window->visible() && collision(_mouse, window->rect())) {
                        window->onLeftMouseUp(_mouse);
                        mouseUpOnWindow = true;
                        break;
                    }
                for (Element *element : _ui)
                    if (!mouseUpOnWindow &&
                        element->visible() &&
                        collision(_mouse, element->rect())) {
                        element->onLeftMouseUp(_mouse);
                        break;
                    }

                // Dragged item onto map -> drop.
                if (!mouseUpOnWindow && Container::getDragItem()) {
                    Container::dropItem();
                }

                // Mouse down and up on same entity: onLeftClick
                if (_leftMouseDownEntity && _currentMouseOverEntity == _leftMouseDownEntity)
                    _currentMouseOverEntity->onLeftClick(*this);
                _leftMouseDownEntity = 0;

                break;
            }

            case SDL_BUTTON_RIGHT:
                _rightMouseDown = false;

                // Items can only be constructed or used from the inventory, not container
                // objects.
                if (_inventoryWindow->visible()) {
                    _inventoryWindow->onRightMouseUp(_mouse);
                    const Item *useItem = Container::getUseItem();
                    if (useItem)
                        _constructionFootprint = useItem->constructsObject()->image();
                    else
                        _constructionFootprint = Texture();
                }

                // Mouse down and up on same entity: onRightClick
                if (_rightMouseDownEntity && _currentMouseOverEntity == _rightMouseDownEntity)
                    _currentMouseOverEntity->onRightClick(*this);
                _rightMouseDownEntity = 0;

                break;
            }

            break;

        case SDL_MOUSEWHEEL:
            if (e.wheel.y < 0) {
                for (Window *window : _windows)
                    if (collision(_mouse, window->rect()))
                        window->onScrollDown(_mouse);
                for (Element *element : _ui)
                    if (collision(_mouse, element->rect()))
                        element->onScrollDown(_mouse);
            } else if (e.wheel.y > 0) {
                for (Window *window : _windows)
                    if (collision(_mouse, window->rect()))
                        window->onScrollUp(_mouse);
                for (Element *element : _ui)
                    if (collision(_mouse, element->rect()))
                        element->onScrollUp(_mouse);
            }
            break;

        case SDL_WINDOWEVENT:
            switch(e.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                renderer.updateSize();
                renderer.setScale(static_cast<float>(renderer.width()) / SCREEN_X,
                                    static_cast<float>(renderer.height()) / SCREEN_Y);
                for (Window *window : _windows)
                    window->forceRefresh();
                for (Element *element : _ui)
                    element->forceRefresh();
                break;
            }

        default:
            // Unhandled event
            ;
        }
    }
    // Poll keys (whether they are currently pressed; not key events)
    static const Uint8 *keyboardState = SDL_GetKeyboardState(0);
    if (_loggedIn && !SDL_IsTextInputActive()) {
        bool
            up = keyboardState[SDL_SCANCODE_UP] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_W] == SDL_PRESSED,
            down = keyboardState[SDL_SCANCODE_DOWN] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_S] == SDL_PRESSED,
            left = keyboardState[SDL_SCANCODE_LEFT] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_A] == SDL_PRESSED,
            right = keyboardState[SDL_SCANCODE_RIGHT] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_D] == SDL_PRESSED;
        if (up != down || left != right) {
            static const double DIAG_SPEED = MOVEMENT_SPEED / SQRT_2;
            const double
                dist = delta * MOVEMENT_SPEED,
                diagDist = delta * DIAG_SPEED;
            Point newLoc = _pendingCharLoc;
            if (up != down) {
                if (up && !down)
                    newLoc.y -= (left != right) ? diagDist : dist;
                else if (down && !up)
                    newLoc.y += (left != right) ? diagDist : dist;
            }
            if (left && !right)
                newLoc.x -= (up != down) ? diagDist : dist;
            else if (right && !left)
                newLoc.x += (up != down) ? diagDist : dist;

            _pendingCharLoc = newLoc;
            static double const MAX_PENDING_DISTANCE = 50;
            _pendingCharLoc = interpolate(_character.location(), _pendingCharLoc,
                                            MAX_PENDING_DISTANCE);
            _mouseMoved = true;
        }
    }
}
