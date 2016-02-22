// (C) 2015 Tim Gurto

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <set>
#include <sstream>
#include <vector>

#include "Client.h"
#include "EntityType.h"
#include "LogSDL.h"
#include "Renderer.h"
#include "TooltipBuilder.h"
#include "ui/Button.h"
#include "ui/Container.h"
#include "ui/Element.h"
#include "../XmlReader.h"
#include "../messageCodes.h"
#include "../util.h"
#include "../server/User.h"

extern Args cmdLineArgs;
extern Renderer renderer;

// TODO: Move all client functionality to a namespace, rather than a class.
Client *Client::_instance = 0;

LogSDL *Client::_debugInstance = 0;

const int Client::SCREEN_X = 640;
const int Client::SCREEN_Y = 360;

const size_t Client::BUFFER_SIZE = 1023;

const Uint32 Client::MAX_TICK_LENGTH = 100;
const Uint32 Client::SERVER_TIMEOUT = 10000;
const Uint32 Client::CONNECT_RETRY_DELAY = 3000;
const Uint32 Client::PING_FREQUENCY = 5000;

const Uint32 Client::TIME_BETWEEN_LOCATION_UPDATES = 50;

const int Client::ICON_SIZE = 16;
const size_t Client::ICONS_X = 8;
Rect Client::INVENTORY_RECT;
const int Client::HEADING_HEIGHT = 14;
const int Client::LINE_GAP = 6;

const int Client::TILE_W = 32;
const int Client::TILE_H = 32;
const double Client::MOVEMENT_SPEED = 80;

const int Client::ACTION_DISTANCE = 30;

const size_t Client::INVENTORY_SIZE = 10;

const size_t Client::MAX_TEXT_ENTERED = 100;

const int Client::PLAYER_ACTION_CHANNEL = 0;

bool Client::isClient = false;

Client::Client():
_cursorNormal(std::string("Images/Cursors/normal.png"), Color::MAGENTA),
_cursorGather(std::string("Images/Cursors/gather.png"), Color::MAGENTA),
_cursorContainer(std::string("Images/Cursors/container.png"), Color::MAGENTA),
_currentCursor(&_cursorNormal),

_activeRecipe(0),
_recipeList(0),
_detailsPane(0),
_craftingWindow(0),
_inventoryWindow(0),

_actionTimer(0),
_actionLength(0),

_loop(true),
_socket(),

_defaultFont(0),
_defaultFontOffset(0),

_mouse(0,0),
_mouseMoved(false),
_leftMouseDown(false),
_leftMouseDownEntity(0),
_rightMouseDown(false),
_rightMouseDownEntity(0),

_time(SDL_GetTicks()),
_timeElapsed(0),
_lastPingReply(_time),
_lastPingSent(_time),
_latency(0),
_timeSinceConnectAttempt(CONNECT_RETRY_DELAY),

_invalidUsername(false),
_loggedIn(false),
_loaded(false),

_timeSinceLocUpdate(0),

_tooltipNeedsRefresh(false),

_mapX(0), _mapY(0),

_currentMouseOverEntity(0),

_debug(360/13, "client.log", "04B_03__.TTF", 8){
    isClient = true;
    _instance = this;
    _debugInstance = &_debug;


    // Read config file
    XmlReader xr("client-config.xml");

    std::string fontFile = "04B_03__.TTF";
    int fontSize = 8;
    int fontHeight = 8;
    auto elem = xr.findChild("debugFont");
    if (xr.findAttr(elem, "filename", fontFile) ||
        xr.findAttr(elem, "size", fontSize))
        _debug.setFont(fontFile, fontSize);
    if (xr.findAttr(elem, "height", fontHeight)) _debug.setLineHeight(fontHeight);

    fontFile = "poh_pixels.ttf";
    fontSize = 16;
    elem = xr.findChild("gameFont");
    xr.findAttr(elem, "filename", fontFile);
    xr.findAttr(elem, "size", fontSize);
    _defaultFont = TTF_OpenFont(fontFile.c_str(), fontSize);
    Element::font(_defaultFont);
    xr.findAttr(elem, "offset", _defaultFontOffset);
    Element::textOffset = _defaultFontOffset;
    xr.findAttr(elem, "height", Element::TEXT_HEIGHT);

    int castBarY = 300, castBarW = 150, castBarH = 11;
    elem = xr.findChild("castBar");
    xr.findAttr(elem, "y", castBarY);
    xr.findAttr(elem, "w", castBarW);
    xr.findAttr(elem, "h", castBarH);


    Element::initialize();

    SDL_ShowCursor(SDL_DISABLE);

    _debug << cmdLineArgs << Log::endl;
    Socket::debug = &_debug;

    int ret = (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 512) < 0);
    if (ret < 0){
        _debug("SDL_mixer failed to initialize.", Color::RED);
    } else {
        _debug("SDL_mixer initialized.");
    }

    renderer.setDrawColor();

    _entities.insert(&_character);

    Avatar::image("Images/man.png");

    // Load terrain
    xr.newFile("Data/terrain.xml");
    for (auto elem : xr.getChildren("terrain")) {
        int index;
        if (!xr.findAttr(elem, "index", index))
            continue;
        std::string fileName;
        if (!xr.findAttr(elem, "imageFile", fileName))
            continue;
        int isTraversable = 1;
        xr.findAttr(elem, "isTraversable", isTraversable);
        if (index >= static_cast<int>(_terrain.size()))
            _terrain.resize(index+1);
        int frames = 1, frameTime = 0;
        xr.findAttr(elem, "frames", frames);
        xr.findAttr(elem, "frameTime", frameTime);
        _terrain[index] = Terrain(fileName, isTraversable != 0, frames, frameTime);
    }

    // Player's inventory
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory.push_back(std::make_pair<const Item *, size_t>(0, 0));

    _invLabel = Texture(_defaultFont, "Inventory");
    int invW =  max(min(ICONS_X, _inventory.size()) * (ICON_SIZE + 1) + 1,
                    static_cast<unsigned>(_invLabel.width()));
    int invH = ICON_SIZE + _invLabel.height() + 1;
    INVENTORY_RECT = Rect(SCREEN_X - invW, SCREEN_Y - invH, invW, invH);

    // Randomize player name if not supplied
    if (cmdLineArgs.contains("username"))
        _username = cmdLineArgs.getString("username");
    else
        for (int i = 0; i != 3; ++i)
            _username.push_back('a' + rand() % 26);
    _debug << "Player name: " << _username << Log::endl;
    _character.name(_username);

    SDL_StopTextInput();

    // Load Items
    xr.newFile("Data/items.xml");
    for (auto elem : xr.getChildren("item")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
            continue; // ID and name are mandatory.
        Item item(id, name);
        std::string s;
        for (auto child : xr.getChildren("class", elem))
            if (xr.findAttr(child, "name", s)) item.addClass(s);

        if (xr.findAttr(elem, "iconFile", s))
            item.icon(s);
        else
            item.icon(id);

        if (xr.findAttr(elem, "constructs", s))
            // Create dummy ObjectType if necessary
            item.constructsObject(&*(_objectTypes.insert(ClientObjectType(s)).first));
        
        std::pair<std::set<Item>::iterator, bool> ret = _items.insert(item);
        if (!ret.second) {
            Item &itemInPlace = const_cast<Item &>(*ret.first);
            itemInPlace = item;
        }
    }

    // Recipes
    xr.newFile("Data/recipes.xml");
    for (auto elem : xr.getChildren("recipe")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id))
            continue; // ID is mandatory.
        Recipe recipe(id);

        std::string s;
        if (!xr.findAttr(elem, "product", s))
            continue; // product is mandatory.
        auto it = _items.find(s);
        if (it == _items.end()) {
            _debug << Color::RED << "Skipping recipe with invalid product " << s << Log::endl;
            continue;
        }
        recipe.product(&*it);

        for (auto child : xr.getChildren("material", elem)) {
            int matQty = 1;
            xr.findAttr(child, "quantity", matQty);
            if (xr.findAttr(child, "id", s)) {
                auto it = _items.find(Item(s));
                if (it == _items.end()) {
                    _debug << Color::RED << "Skipping invalid recipe material " << s << Log::endl;
                    continue;
                }
                recipe.addMaterial(&*it, matQty);
            }
        }

        for (auto child : xr.getChildren("tool", elem)) {
            if (xr.findAttr(child, "name", s)) {
                recipe.addTool(s);
            }
        }
        
        _recipes.insert(recipe);
    }

    // Object types
    xr.newFile("Data/objectTypes.xml");
    for (auto elem : xr.getChildren("objectType")) {
        std::string s; int n;
        if (!xr.findAttr(elem, "id", s))
            continue;
        ClientObjectType cot(s);
        xr.findAttr(elem, "imageFile", s); // If no explicit imageFile, s will still == id
        cot.image(std::string("Images/Objects/") + s + ".png");
        if (xr.findAttr(elem, "name", s)) cot.name(s);
        Rect drawRect(0, 0, cot.width(), cot.height());
        bool
            xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
            ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
        if (xSet || ySet)
            cot.drawRect(drawRect);
        if (xr.getChildren("yield", elem).size() > 0) cot.canGather(true);
        
        auto container = xr.findChild("container", elem);
        if (container) {
            if (xr.findAttr(container, "slots", n)) cot.containerSlots(n);
        }

        if (xr.findAttr(elem, "isFlat", n) && n != 0) cot.isFlat(true);
        if (xr.findAttr(elem, "gatherSound", s))
            cot.gatherSound(std::string("Sounds/") + s + ".wav");
        auto collisionRect = xr.findChild("collisionRect", elem);
        if (collisionRect) {
            Rect r;
            xr.findAttr(collisionRect, "x", r.x);
            xr.findAttr(collisionRect, "y", r.y);
            xr.findAttr(collisionRect, "w", r.w);
            xr.findAttr(collisionRect, "h", r.h);
            cot.collisionRect(r);
        }
        auto pair = _objectTypes.insert(cot);
        if (!pair.second) {
            ClientObjectType &type = const_cast<ClientObjectType &>(*pair.first);
            type = cot;
        }
    }


    Element::absMouse = &_mouse;

    initializeCraftingWindow();
    initializeInventoryWindow();
    addWindow(_craftingWindow);
    addWindow(_inventoryWindow);
    
    _castBar = new ProgressBar<Uint32>(Rect(SCREEN_X/2 - castBarW/2, castBarY, castBarW, castBarH),
                                       _actionTimer, _actionLength);
    _castBar->hide();
    addUI(_castBar);

    static const int
        MENU_BUTTON_W = 50,
        MENU_BUTTON_H = 13,
        NUM_BUTTONS = 2;
    Element *menuBar = new Element(Rect(SCREEN_X/2 - MENU_BUTTON_W * NUM_BUTTONS / 2,
                                        SCREEN_Y - MENU_BUTTON_H,
                                        MENU_BUTTON_W * NUM_BUTTONS,
                                        MENU_BUTTON_H));
    menuBar->addChild(new Button(Rect(0, 0, MENU_BUTTON_W, MENU_BUTTON_H), "Crafting",
                                 Element::toggleVisibilityOf, _craftingWindow));
    menuBar->addChild(new Button(Rect(MENU_BUTTON_W, 0, MENU_BUTTON_W, MENU_BUTTON_H), "Inventory",
                                 Element::toggleVisibilityOf, _inventoryWindow));
    addUI(menuBar);
}

Client::~Client(){
    SDL_ShowCursor(SDL_ENABLE);
    Element::cleanup();
    if (_defaultFont)
        TTF_CloseFont(_defaultFont);
    Avatar::image("");
    for (const Entity *entityConst : _entities) {
        Entity *entity = const_cast<Entity *>(entityConst);
        if (entity != &_character)
            delete entity;
    }

    // Some entities will destroy their own windows, and remove them from this list.
    for (Window *window : _windows)
        delete window;

    for (Element *element : _ui)
        delete element;
    Mix_Quit();
}

void Client::checkSocket(){
    if (_invalidUsername)
        return;

    // Ensure connected to server
    if (!_loggedIn && _timeSinceConnectAttempt >= CONNECT_RETRY_DELAY) {
        _timeSinceConnectAttempt = 0;
        // Server details
        sockaddr_in serverAddr;
        serverAddr.sin_addr.s_addr = cmdLineArgs.contains("server-ip") ?
                                     inet_addr(cmdLineArgs.getString("server-ip").c_str()) :
                                     inet_addr("127.0.0.1");
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = cmdLineArgs.contains("server-port") ?
                              cmdLineArgs.getInt("server-port") :
                              htons(8888);
        if (connect(_socket.getRaw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0) {
            _debug << Color::RED << "Connection error: " << WSAGetLastError() << Log::endl;
        } else {
            _debug("Connected to server", Color::GREEN);
            // Announce player name
            sendMessage(CL_I_AM, _username);
            sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
        }
    }

    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        static char buffer[BUFFER_SIZE+1];
        int charsRead = recv(_socket.getRaw(), buffer, BUFFER_SIZE, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0){
            buffer[charsRead] = '\0';
            _messages.push(std::string(buffer));
        }
    }
}

void Client::run(){

    Uint32 timeAtLastTick = SDL_GetTicks();
    while (_loop) {
        _time = SDL_GetTicks();

        // Send ping
        if (_loggedIn && _time - _lastPingSent > PING_FREQUENCY) {
            sendMessage(CL_PING, makeArgs(_time));
            _lastPingSent = _time;
        }

        _timeElapsed = _time - timeAtLastTick;
        if (_timeElapsed > MAX_TICK_LENGTH)
            _timeElapsed = MAX_TICK_LENGTH;
        const double delta = _timeElapsed / 1000.0;
        timeAtLastTick = _time;

        // Ensure server connectivity
        if (_loggedIn && _time - _lastPingReply > SERVER_TIMEOUT) {
            _debug("Disconnected from server", Color::RED);
            _socket = Socket();
            _loggedIn = false;
        }

        if (!_loggedIn) {
            _timeSinceConnectAttempt += _timeElapsed;

        } else { // Update server with current location
            const bool atTarget = _pendingCharLoc == _character.location();
            if (atTarget)
                _timeSinceLocUpdate = 0;
            else {
                _timeSinceLocUpdate += _timeElapsed;
                if (_timeSinceLocUpdate > TIME_BETWEEN_LOCATION_UPDATES){
                    sendMessage(CL_LOCATION, makeArgs(_pendingCharLoc.x, _pendingCharLoc.y));
                    _tooltipNeedsRefresh = true;
                    _timeSinceLocUpdate = 0;
                }
            }
        }

        // Deal with any messages from the server
        if (!_messages.empty()){
            handleMessage(_messages.front());
            _messages.pop();
        }

        // Handle user events
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
                            if (_enteredText.at(0) == '[') {
                                // Send message to server
                                sendRawMessage(_enteredText);
                                _debug(_enteredText, Color::YELLOW);
                            } else {
                                _debug(_enteredText, Color::WHITE);
                            }
                            _enteredText = "";
                        }
                        break;
                    }

                } else {
                    // Regular key input

                    switch(e.key.keysym.sym) {

                    case SDLK_ESCAPE:
                        if (_actionLength != 0)
                            sendMessage(CL_CANCEL_ACTION);
                        else if (_craftingWindow->visible())
                            _craftingWindow->hide();
                        else
                            _loop = false;
                        break;

                    case SDLK_LEFTBRACKET:
                        SDL_StartTextInput();
                        _enteredText = "[";
                        break;

                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        SDL_StartTextInput();
                        break;

                    case SDLK_c:
                        _craftingWindow->toggleVisibility();
                        break;

                    case SDLK_i:
                        _inventoryWindow->toggleVisibility();
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
                        if (element->visible())
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
                        if (element->visible())
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
                if (e.wheel.y < 0)
                    _craftingWindow->onScrollDown(_mouse);
                else if (e.wheel.y > 0)
                    _craftingWindow->onScrollUp(_mouse);
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

        // Update entities
        std::vector<Entity *> entitiesToReorder;
        for (Entity::set_t::iterator it = _entities.begin(); it != _entities.end(); ) {
            Entity::set_t::iterator next = it;
            ++next;
            Entity *const toUpdate = *it;
            toUpdate->update(delta);
            if (toUpdate->yChanged()) {
                // Entity has moved up or down, and must be re-ordered in set.
                entitiesToReorder.push_back(toUpdate);
                _entities.erase(it);
                toUpdate->yChanged(false);
            }
            it = next;
        }
        for (Entity *entity : entitiesToReorder)
            _entities.insert(entity);
        entitiesToReorder.clear();

        updateOffset();

        // Update cast bar
        if (_actionLength > 0) {
            _actionTimer = min(_actionTimer + _timeElapsed, _actionLength);
            _castBar->show();
        }

        // Update terrain animation
        for (Terrain &terrain : _terrain)
            terrain.advanceTime(_timeElapsed);

        if (_mouseMoved)
            checkMouseOver();

        checkSocket();
        // Draw
        draw();
        SDL_Delay(5);
    }
}

Entity *Client::getEntityAtMouse(){
    const Point mouseOffset = _mouse - _offset;
    Entity::set_t::iterator mouseOverIt = _entities.end();
    static const int LOOKUP_MARGIN = 30;
    Entity
        topEntity(0, Point(0, mouseOffset.y - LOOKUP_MARGIN)),
        bottomEntity(0, Point(0, mouseOffset.y + LOOKUP_MARGIN));
    auto
        lowerBound = _entities.lower_bound(&topEntity),
        upperBound = _entities.upper_bound(&bottomEntity);
    for (auto it = lowerBound; it != upperBound; ++it) {
        if (*it != &_character &&(*it)->collision(mouseOffset))
            mouseOverIt = it;
    }
    if (mouseOverIt != _entities.end())
        return *mouseOverIt;
    else
        return 0;
}

void Client::checkMouseOver(){
    _currentCursor = &_cursorNormal;

    // Check if mouse is over an entity
    const Entity *const oldMouseOverEntity = _currentMouseOverEntity;
    _currentMouseOverEntity = getEntityAtMouse();
    if (!_currentMouseOverEntity)
        return;
    if (_currentMouseOverEntity != oldMouseOverEntity ||
        _currentMouseOverEntity->needsTooltipRefresh() ||
        _tooltipNeedsRefresh) {
        _currentMouseOverEntity->refreshTooltip(*this);
        _tooltipNeedsRefresh = false;
    }
    
    // Set cursor
    if (_currentMouseOverEntity->isObject()) {
        const ClientObjectType &objType =
            *dynamic_cast<ClientObject*>(_currentMouseOverEntity)->objectType();
        if (objType.canGather())
            _currentCursor = &_cursorGather;
        else if (objType.containerSlots() != 0)
            _currentCursor = &_cursorContainer;
    }
}

void Client::draw() const{
    if (!_loggedIn || !_loaded){
        renderer.setDrawColor(Color::BLACK);
        renderer.clear();
        _debug.draw();
        renderer.present();
        return;
    }

    // Background
    renderer.setDrawColor(Color::BLUE_HELL);
    renderer.clear();

    // Map
    size_t
        xMin = static_cast<size_t>(max<double>(0, -offset().x / TILE_W)),
        xMax = static_cast<size_t>(min<double>(_mapX,
                                               1.0 * (-offset().x + SCREEN_X) / TILE_W + 1.5)),
        yMin = static_cast<size_t>(max<double>(0, -offset().y / TILE_H)),
        yMax = static_cast<size_t>(min<double>(_mapY, (-offset().y + SCREEN_Y) / TILE_H + 1));
    for (size_t y = yMin; y != yMax; ++y) {
        const int yLoc = y * TILE_H + toInt(offset().y);
        for (size_t x = xMin; x != xMax; ++x){
            int xLoc = x * TILE_W + toInt(offset().x);
            if (y % 2 == 1)
                xLoc -= TILE_W/2;
            drawTile(x, y, xLoc, yLoc);
        }
    }

    // Character's target and actual location
    if (isDebug()) {
        renderer.setDrawColor(Color::CYAN);
        const Point &actualLoc = _character.destination() + offset();
        renderer.drawRect(Rect(actualLoc.x - 1, actualLoc.y - 1, 3, 3));

        renderer.setDrawColor(Color::WHITE);
        Point pendingLoc(_pendingCharLoc.x + offset().x, _pendingCharLoc.y + offset().y);
        renderer.drawRect(Rect(pendingLoc.x, pendingLoc.y, 1, 1));
        renderer.drawRect(Rect(pendingLoc.x - 2, pendingLoc.y - 2, 5, 5));
    }

    // Entities, sorted from back to front
    static const int
        DRAW_MARGIN_ABOVE = 50,
        DRAW_MARGIN_BELOW = 10,
        DRAW_MARGIN_SIDES = 30;
    const double
        topY = -offset().y - DRAW_MARGIN_BELOW,
        bottomY = -offset().y + SCREEN_Y + DRAW_MARGIN_ABOVE,
        leftX = -offset().x - DRAW_MARGIN_SIDES,
        rightX = -offset().x + SCREEN_X + DRAW_MARGIN_SIDES;
    // Cull by y
    Entity
        topEntity(0, Point(0, topY)),
        bottomEntity(0, Point(0, bottomY));
    auto top = _entities.lower_bound(&topEntity);
    auto bottom = _entities.upper_bound(&bottomEntity);
    // Flat entities
    for (auto it = top; it != bottom; ++it) {
        if ((*it)->type()->isFlat()) {
            // Cull by x
            double x = (*it)->location().x;
            if (x >= leftX && x <= rightX)
                (*it)->draw(*this);
        }
    }
    // Non-flat entities
    for (auto it = top; it != bottom; ++it) {
        if (!(*it)->type()->isFlat()) {
            // Cull by x
            double x = (*it)->location().x;
            if (x >= leftX && x <= rightX)
                (*it)->draw(*this);
        }
    }

    // FPS/latency
    std::ostringstream oss;
    if (_timeElapsed > 0)
        oss << toInt(1000.0/_timeElapsed);
    else
        oss << "infinite ";
    oss << "fps " << _latency << "ms";
    const Texture statsDisplay(_defaultFont, oss.str(), Color::YELLOW);
    statsDisplay.draw(SCREEN_X - statsDisplay.width(), 0);

    // Text box
    if (SDL_IsTextInputActive()) {
        static const int
            TEXT_BOX_HEIGHT = 13,
            TEXT_BOX_WIDTH = 300;
        static const Rect TEXT_BOX_RECT((SCREEN_X - TEXT_BOX_WIDTH) / 2,
                                        (SCREEN_Y - TEXT_BOX_HEIGHT) / 2,
                                        TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT);
        static const Color TEXT_BOX_BORDER = Color::GREY_4;
        renderer.setDrawColor(TEXT_BOX_BORDER);
        renderer.drawRect(TEXT_BOX_RECT + Rect(-1, -1, 2, 2));
        renderer.setDrawColor(Color::BLACK);
        renderer.fillRect(TEXT_BOX_RECT);
        const Texture text(_defaultFont, _enteredText);
        static const int MAX_TEXT_WIDTH = TEXT_BOX_WIDTH - 2;
        int cursorX;
        if (text.width() < MAX_TEXT_WIDTH) {
            text.draw(TEXT_BOX_RECT.x + 1, TEXT_BOX_RECT.y);
            cursorX = TEXT_BOX_RECT.x + text.width() + 1;
        } else {
            const Rect
                dstRect(TEXT_BOX_RECT.x + 1, TEXT_BOX_RECT.y, MAX_TEXT_WIDTH, text.height()),
                srcRect = Rect(text.width() - MAX_TEXT_WIDTH, 0,
                                   MAX_TEXT_WIDTH, text.height());
            text.draw(dstRect, srcRect);
            cursorX = TEXT_BOX_RECT.x + TEXT_BOX_WIDTH;
        }
        renderer.setDrawColor(Color::WHITE);
        renderer.fillRect(Rect(cursorX, TEXT_BOX_RECT.y + 1, 1, TEXT_BOX_HEIGHT - 2));
    }

    // Non-window UI
    for (Element *element : _ui)
        element->draw();

    // Windows
    for (windows_t::const_reverse_iterator it = _windows.rbegin(); it != _windows.rend(); ++it)
        (*it)->draw();

    // Dragged item
    static const Point MOUSE_ICON_OFFSET(-Client::ICON_SIZE/2, -Client::ICON_SIZE/2);
    const Item *draggedItem = Container::getDragItem();
    if (draggedItem)
        draggedItem->icon().draw(_mouse + MOUSE_ICON_OFFSET);

    // Used item
    if (_constructionFootprint) {
        const ClientObjectType *ot = Container::getUseItem()->constructsObject();
        Rect footprintRect = ot->collisionRect() + _mouse - _offset;
        if (distance(playerCollisionRect(), footprintRect) <= Client::ACTION_DISTANCE) {
            renderer.setDrawColor(Color::WHITE);
            renderer.fillRect(footprintRect + _offset);

            const Rect &drawRect = ot->drawRect();
            int
                x = toInt(_mouse.x + drawRect.x),
                y = toInt(_mouse.y + drawRect.y);
            _constructionFootprint.setAlpha(0x7f);
            _constructionFootprint.draw(x, y);
            _constructionFootprint.setAlpha();
        } else {
            renderer.setDrawColor(Color::RED);
            renderer.fillRect(footprintRect + _offset);
        }
    }

    // Tooltip
    drawTooltip();

    // Cursor
    _currentCursor->draw(_mouse);

    _debug.draw();
    renderer.present();
}

Texture Client::getInventoryTooltip() const{
    if (_mouse.y < INVENTORY_RECT.w + _invLabel.height())
        return Texture();
    const int slot = static_cast<size_t>((_mouse.x - INVENTORY_RECT.x) / (ICON_SIZE + 1));
    if (slot < 0 || static_cast<size_t>(slot) >= _inventory.size())
        return Texture();

    const Item *const item = _inventory[static_cast<size_t>(slot)].first;
    if (!item)
        return Texture();
    TooltipBuilder tb;
    tb.setColor(Color::WHITE);
    tb.addLine(item->name());
    if (item->hasClasses()) {
        tb.setColor();
        tb.addGap();
        for (const std::string &className : item->classes()) {
            tb.addLine(className);
        }
    }
    if (item->constructsObject()) {
        tb.addGap();
        tb.setColor(Color::YELLOW);
        tb.addLine("Right-click to construct");
    }
    return tb.publish();
}

void Client::drawTooltip() const{
    const Texture *tooltip;
    if (Element::tooltip())
        tooltip = Element::tooltip();
    else if (_currentMouseOverEntity)
        tooltip = &_currentMouseOverEntity->tooltip();
    else
        return;

    if (tooltip) {
        static const int EDGE_GAP = 2; // Gap from screen edges
        static const int CURSOR_GAP = 10; // Horizontal gap from cursor
        int x, y;
        const int mouseX = toInt(_mouse.x);
        const int mouseY = toInt(_mouse.y);

        // y: below cursor, unless too close to the bottom of the screen
        if (SCREEN_Y > mouseY + tooltip->height() + EDGE_GAP)
            y = mouseY;
        else
            y = SCREEN_Y - tooltip->height() - EDGE_GAP;

        // x: to the right of the cursor, unless too close to the right of the screen
        if (SCREEN_X > mouseX + tooltip->width() + EDGE_GAP + CURSOR_GAP)
            x = mouseX + CURSOR_GAP;
        else
            x = mouseX - tooltip->width() - CURSOR_GAP;
        tooltip->draw(x, y);
    }
}

void Client::drawTile(size_t x, size_t y, int xLoc, int yLoc) const{
    if (isDebug()) {
        _terrain[_map[x][y]].draw(xLoc, yLoc);
        return;
    }


    /*
          H | E
      L | tileID| R
          G | F
    */
    const Rect drawLoc(xLoc, yLoc, 0, 0);
    const bool yOdd = (y % 2 == 1);
    size_t tileID, L, R, E, F, G, H;
    tileID = _map[x][y];
    R = x == _mapX-1 ? tileID : _map[x+1][y];
    L = x == 0 ? tileID : _map[x-1][y];
    if (y == 0) {
        H = E = tileID;
    } else {
        if (yOdd) {
            E = _map[x][y-1];
            H = x == 0 ? tileID : _map[x-1][y-1];
        } else {
            E = x == _mapX-1 ? tileID : _map[x+1][y-1];
            H = _map[x][y-1];
        }
    }
    if (y == _mapY-1) {
        G = F = tileID;
    } else {
        if (!yOdd) {
            F = x == _mapX-1 ? tileID : _map[x+1][y+1];
            G = _map[x][y+1];
        } else {
            F = _map[x][y+1];
            G = x == 0 ? tileID : _map[x-1][y+1];
        }
    }

    static const Rect
        TOP_LEFT     (0,        0,        TILE_W/2, TILE_H/2),
        TOP_RIGHT    (TILE_W/2, 0,        TILE_W/2, TILE_H/2),
        BOTTOM_LEFT  (0,        TILE_H/2, TILE_W/2, TILE_H/2),
        BOTTOM_RIGHT (TILE_W/2, TILE_H/2, TILE_W/2, TILE_H/2),
        LEFT_HALF    (0,        0,        TILE_W/2, TILE_H),
        RIGHT_HALF   (TILE_W/2, 0,        TILE_W/2, TILE_H),
        FULL         (0,        0,        TILE_W,   TILE_H);

    // Black background
    // Assuming all tile images are set to SDL_BLENDMODE_ADD and quarter alpha
    renderer.setDrawColor(Color::BLACK);
    if (yOdd && x == 0) {
        renderer.fillRect(drawLoc + RIGHT_HALF);
    }
    else if (!yOdd && x == _mapX-1) {
        renderer.fillRect(drawLoc + LEFT_HALF);
    }
    else {
        renderer.fillRect(drawLoc + FULL);
    }

    // Half-alpha base tile
    _terrain[tileID].setHalfAlpha();
    if (yOdd && x == 0) {
        _terrain[tileID].draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain[tileID].draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
    } else if (!yOdd && x == _mapX-1) {
        _terrain[tileID].draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain[tileID].draw(drawLoc + TOP_LEFT, TOP_LEFT);
    } else {
        _terrain[tileID].draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain[tileID].draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
        _terrain[tileID].draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain[tileID].draw(drawLoc + TOP_LEFT, TOP_LEFT);
    }
    _terrain[tileID].setQuarterAlpha();

    // Quarter-alpha L, R, E, F, G, H tiles
    if (!yOdd || x != 0) {
        _terrain[L].draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain[L].draw(drawLoc + TOP_LEFT, TOP_LEFT);
        _terrain[G].draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain[H].draw(drawLoc + TOP_LEFT, TOP_LEFT);
    }
    if (yOdd || x != _mapX-1) {
        _terrain[R].draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain[R].draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
        _terrain[E].draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain[F].draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
    }

    /*if (!_terrain[tileID].isTraversable()) {
        renderer.setDrawColor(Color::RED);
        renderer.drawRect(drawLoc + FULL);
    }*/
}

void Client::startCrafting(void *data){
    if (_instance->_activeRecipe) {
        _instance->sendMessage(CL_CRAFT, _instance->_activeRecipe->id());
        _instance->prepareAction("Crafting " + _instance->_activeRecipe->product()->name());
    }
}

bool Client::playerHasItem(const Item *item, size_t quantity) const{
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        const std::pair<const Item *, size_t> slot = _inventory[i];
        if (slot.first == item) {
            if (slot.second >= quantity)
                return true;
            else
                quantity -= slot.second;
        }
    }
    return false;
}

//bool Client::playerHasTool(const std::string &className) const{
//    // Check inventory
//    for (std::pair<const Item *, size_t> slot : _inventory)
//        if slot.first->isClass(className)
//            return true;
//
//    //Check nearby objects
//    for (
//    return false;
//}

void Client::handleMessage(const std::string &msg){
    _partialMessage.append(msg);
    std::istringstream iss(_partialMessage);
    _partialMessage = "";
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];

    // Read while there are new messages
    while (!iss.eof()) {
        // Discard malformed data
        if (iss.peek() != '[') {
            iss.get(buffer, BUFFER_SIZE, '[');
            _debug << "Read " << iss.gcount() << " characters." << Log::endl;
            _debug << Color::RED << "Malformed message; discarded \""
                   << buffer << "\"" << Log::endl;
            if (iss.eof()) {
                break;
            }
        }

        // Get next message
        iss.get(buffer, BUFFER_SIZE, ']');
        if (iss.eof()){
            _partialMessage = buffer;
            break;
        } else {
            std::streamsize charsRead = iss.gcount();
            buffer[charsRead] = ']';
            buffer[charsRead+1] = '\0';
            iss.ignore(); // Throw away ']'
        }
        std::istringstream singleMsg(buffer);
        //_debug(buffer, Color::CYAN);
        singleMsg >> del >> msgCode >> del;
        switch(msgCode) {

        case SV_WELCOME:
        {
            if (del != ']')
                break;
            _loggedIn = true;
            _timeSinceConnectAttempt = 0;
            _lastPingSent = _lastPingReply = _time;
            _debug("Successfully logged in to server", Color::GREEN);
            break;
        }

        case SV_PING_REPLY:
        {
            Uint32 timeSent;
            singleMsg >> timeSent >> del;
            if (del != ']')
                break;
            _lastPingReply = _time;
            _latency = (_time - timeSent) / 2;
            break;
        }

        case SV_USER_DISCONNECTED:
        {
            std::string name;
            singleMsg.get(buffer, BUFFER_SIZE, ']');
            name = std::string(buffer);
            singleMsg >> del;
            if (del != ']')
                break;
            const std::map<std::string, Avatar*>::iterator it = _otherUsers.find(name);
            if (it != _otherUsers.end()) {
                removeEntity(it->second);
                _otherUsers.erase(it);
            }
            _debug << name << " disconnected." << Log::endl;
            break;
        }

        case SV_DUPLICATE_USERNAME:
            if (del != ']')
                break;
            _invalidUsername = true;
            _debug << Color::RED << "The user " << _username
                   << " is already connected to the server." << Log::endl;
            break;

        case SV_INVALID_USERNAME:
            if (del != ']')
                break;
            _invalidUsername = true;
            _debug << Color::RED << "The username " << _username << " is invalid." << Log::endl;
            break;

        case SV_SERVER_FULL:
            if (del != ']')
                break;
            _debug("The server is full.  Attempting reconnection...", Color::YELLOW);
            _socket = Socket();
            _loggedIn = false;
            break;

        case SV_TOO_FAR:
            if (del != ']')
                break;
            _debug("You are too far away to perform that action.", Color::YELLOW);
            startAction(0);
            break;

        case SV_DOESNT_EXIST:
            if (del != ']')
                break;
            _debug("That object doesn't exist.", Color::YELLOW);
            startAction(0);
            break;

        case SV_INVENTORY_FULL:
            if (del != ']')
                break;
            _debug("Your inventory is full.", Color::RED);
            startAction(0);
            break;

        case SV_NEED_MATERIALS:
            if (del != ']')
                break;
            _debug("You do not have the necessary materials to create that item.", Color::YELLOW);
            startAction(0);
            break;

        case SV_NEED_TOOLS:
            if (del != ']')
                break;
            _debug("You do not have the necessary tools to create that item.", Color::YELLOW);
            startAction(0);
            break;

        case SV_INVALID_ITEM:
            if (del != ']')
                break;
            _debug("That is not a real item.", Color::RED);
            startAction(0);
            break;

        case SV_CANNOT_CRAFT:
            if (del != ']')
                break;
            _debug("That item cannot be crafted.", Color::RED);
            startAction(0);
            break;

        case SV_ACTION_INTERRUPTED:
            if (del != ']')
                break;
            _debug("Action interrupted.", Color::YELLOW);
            startAction(0);
            break;

        case SV_INVALID_SLOT:
            if (del != ']')
                break;
            _debug("That is not a valid inventory slot.", Color::RED);
            startAction(0);
            break;

        case SV_EMPTY_SLOT:
            if (del != ']')
                break;
            _debug("That inventory slot is empty.", Color::RED);
            startAction(0);
            break;

        case SV_CANNOT_CONSTRUCT:
            if (del != ']')
                break;
            _debug("That item cannot be constructed.", Color::RED);
            startAction(0);
            break;

        case SV_ITEM_NEEDED:
        {
            std::string reqItemClass;
            singleMsg.get(buffer, BUFFER_SIZE, ']');
            reqItemClass = std::string(buffer);
            singleMsg >> del;
            if (del != ']')
                break;
            std::string msg = "You need a";
            const char first = reqItemClass.front();
            if (first == 'a' || first == 'e' || first == 'i' ||
                first == 'o' || first == 'u')
                msg += 'n';
            _debug(msg + ' ' + reqItemClass + " to do that.", Color::YELLOW);
            startAction(0);
            break;
        }

        case SV_BLOCKED:
            if (del != ']')
                break;
            _debug("That location is already occupied.", Color::YELLOW);
            startAction(0);
            break;

        case SV_ACTION_STARTED:
            Uint32 time;
            singleMsg >> time >> del;
            if (del != ']')
                break;
            startAction(time);

            // If crafting, hide footprint now that it has successfully started.
            Container::clearUseItem();
            _constructionFootprint = Texture();

            break;

        case SV_ACTION_FINISHED:
            if (del != ']')
                break;
            startAction(0); // Effectively, hide the cast bar.
            break;

        case SV_MAP_SIZE:
        {
            size_t x, y;
            singleMsg >> x >> del >> y >> del;
            if (del != ']')
                break;
            _mapX = x;
            _mapY = y;
            _map = std::vector<std::vector<size_t> >(_mapX);
            for (size_t x = 0; x != _mapX; ++x)
                _map[x] = std::vector<size_t>(_mapY, 0);
            break;
        }

        case SV_TERRAIN:
        {
            size_t x, y, n;
            singleMsg >> x >> del >> y >> del >> n >> del;
            if (x + n > _mapX)
                break;
            if (y > _mapY)
                break;
            std::vector<size_t> terrain;
            for (size_t i = 0; i != n; ++i) {
                size_t index;
                singleMsg >> index >> del;
                if (index > _terrain.size()) {
                    _debug << Color::RED << "Invalid terrain type receved ("
                           << index << "); aborted." << Log::endl;
                    break;
                }
                terrain.push_back(index);
            }
            if (del != ']')
                break;
            if (terrain.size() != n)
                break;
            for (size_t i = 0; i != n; ++i)
                _map[x+i][y] = terrain[i];
            break;
        }

        case SV_LOCATION:
        {
            std::string name;
            double x, y;
            singleMsg.get(buffer, BUFFER_SIZE, ',');
            name = std::string(buffer);
            singleMsg >> del >> x >> del >> y >> del;
            if (del != ']')
                break;
            const Point p(x, y);
            if (name == _username) {
                if (p.x == _character.location().x)
                    _pendingCharLoc.x = p.x;
                if (p.y == _character.location().y)
                    _pendingCharLoc.y = p.y;
                _character.destination(p);
                if (!_loaded) {
                    setEntityLocation(&_character, p);
                    _pendingCharLoc = p;
                }
                updateOffset();
                _loaded = true;
                _tooltipNeedsRefresh = true;
                _mouseMoved = true;
            } else {
                if (_otherUsers.find(name) == _otherUsers.end()) {
                    // Create new Avatar
                    Avatar *newUser = new Avatar(name, p);
                    _otherUsers[name] = newUser;
                    _entities.insert(newUser);
                }
                _otherUsers[name]->destination(p);
            }
            break;
        }

        case SV_INVENTORY:
        {
            size_t serial, slot, quantity;
            std::string itemID;
            singleMsg >> serial >> del >> slot >> del;
            singleMsg.get(buffer, BUFFER_SIZE, ',');
            itemID = std::string(buffer);
            singleMsg >> del >> quantity >> del;
            if (del != ']')
                break;

            const Item *item = 0;
            if (quantity > 0) {
                std::set<Item>::const_iterator it = _items.find(itemID);
                if (it == _items.end()) {
                    _debug << Color::RED << "Unknown inventory item \"" << itemID
                           << "\"announced; ignored.";
                    break;
                }
                item = &*it;
            }

            Item::vect_t *container;
            ClientObject *object = 0;
            if (serial == 0)
                container = &_inventory;
            else {
                auto it = _objects.find(serial);
                if (it == _objects.end()) {
                    _debug("Received inventory of nonexistent object; ignored.", Color::RED);
                    break;
                }
                object = it->second;
                container = &object->container();
            }
            if (slot >= container->size()) {
                _debug("Received item in invalid inventory slot; ignored.", Color::RED);
                break;
            }
            auto &invSlot = (*container)[slot];
            invSlot.first = item;
            invSlot.second = quantity;
            _recipeList->markChanged();
            if (serial == 0)
                _inventoryWindow->forceRefresh();
            else
                object->refreshWindow();
            break;
        }

        case SV_OBJECT:
        {
            int serial;
            double x, y;
            std::string type;
            singleMsg >> serial >> del >> x >> del >> y >> del;
            singleMsg.get(buffer, BUFFER_SIZE, ']');
            type = std::string(buffer);
            singleMsg >> del;
            if (del != ']')
                break;
            std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()) {
                // A new object was added; add entity to list
                const std::set<ClientObjectType>::const_iterator it = _objectTypes.find(type);
                if (it == _objectTypes.end())
                    break;
                ClientObject *const obj = new ClientObject(serial, &*it, Point(x, y));
                _entities.insert(obj);
                _objects[serial] = obj;
            }
            break;
        }

        case SV_REMOVE_OBJECT:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != ']')
                break;
            const std::map<size_t, ClientObject*>::const_iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Server removed an object we didn't know about.", Color::YELLOW);
                assert(false);
                break; // We didn't know about this object
            }
            if (it->second == _currentMouseOverEntity)
                _currentMouseOverEntity = 0;
            removeEntity(it->second);
            _objects.erase(it);
            break;
        }

        case SV_OWNER:
        {
            int serial;
            singleMsg >> serial >> del;
            singleMsg.get(buffer, BUFFER_SIZE, ']');
            singleMsg >> del;
            if (del != ']')
                break;
            const std::map<size_t, ClientObject*>::iterator it = _objects.find(serial);
            if (it == _objects.end()){
                _debug("Received ownership info for an unknown object.", Color::RED);
                break;
            }
            (it->second)->owner(std::string(buffer));
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }

        if (del != ']' && !iss.eof()) {
            _debug << Color::RED << "Bad message ending. code=" << msgCode << Log::endl;
        }

        iss.peek();
    }
}

void Client::sendRawMessage(const std::string &msg) const{
    _socket.sendMessage(msg);
}

void Client::sendMessage(MessageCode msgCode, const std::string &args) const{
    // Compile message
    std::ostringstream oss;
    oss << '[' << msgCode;
    if (args != "")
        oss << ',' << args;
    oss << ']';

    // Send message
    sendRawMessage(oss.str());
}

const Socket &Client::socket() const{
    return _socket;
}

void Client::removeEntity(Entity *const toRemove){
    const Entity::set_t::iterator it = _entities.find(toRemove);
    if (it != _entities.end())
        _entities.erase(it);
    delete toRemove;
}

TTF_Font *Client::defaultFont() const{
    return _defaultFont;
}

void Client::setEntityLocation(Entity *entity, const Point &location){
    const Entity::set_t::iterator it = _entities.find(entity);
    if (it == _entities.end()){
        assert(false); // Entity is not in set.
        return;
    }
    entity->location(location);
    if (entity->yChanged()) {
        _entities.erase(it);
        _entities.insert(entity);
        entity->yChanged(false);
    }
}

void Client::updateOffset(){
    _offset = Point(SCREEN_X / 2 - _character.location().x,
                    SCREEN_Y / 2 - _character.location().y);
    _intOffset = Point(toInt(_offset.x),
                       toInt(_offset.y));
}

void Client::prepareAction(const std::string &msg){
    _actionMessage = msg;
}

void Client::startAction(Uint32 actionLength){
    _actionTimer = 0;
    _actionLength = actionLength;
    if (actionLength == 0) {
        _castBar->hide();
        Mix_HaltChannel(PLAYER_ACTION_CHANNEL);
    }
}

void Client::addWindow(Window *window){
    _windows.push_front(window);
}

void Client::removeWindow(Window *window){
    _windows.remove(window);
}

void Client::addUI(Element *element){
    _ui.push_back(element);
}
