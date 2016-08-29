// (C) 2015-2016 Tim Gurto

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
#include "TooltipBuilder.h"
#include "ui/Button.h"
#include "ui/Container.h"
#include "ui/Element.h"
#include "ui/LinkedLabel.h"
#include "ui/ProgressBar.h"
#include "ui/TextBox.h"
#include "../XmlReader.h"
#include "../curlUtil.h"
#include "../messageCodes.h"
#include "../util.h"
#include "../server/User.h"

extern Args cmdLineArgs;

// TODO: Move all client functionality to a namespace, rather than a class.
Client *Client::_instance = nullptr;

LogSDL *Client::_debugInstance = nullptr;

const px_t Client::SCREEN_X = 640;
const px_t Client::SCREEN_Y = 360;

const ms_t Client::MAX_TICK_LENGTH = 100;
const ms_t Client::SERVER_TIMEOUT = 10000;
const ms_t Client::CONNECT_RETRY_DELAY = 3000;
const ms_t Client::PING_FREQUENCY = 5000;

const ms_t Client::TIME_BETWEEN_LOCATION_UPDATES = 50;

const px_t Client::ICON_SIZE = 16;
const px_t Client::HEADING_HEIGHT = 14;
const px_t Client::LINE_GAP = 6;

const px_t Client::TILE_W = 32;
const px_t Client::TILE_H = 32;
const double Client::MOVEMENT_SPEED = 80;

const px_t Client::ACTION_DISTANCE = 30;

const px_t Client::CULL_DISTANCE = 100;
const px_t Client::CULL_HYSTERESIS_DISTANCE = 10;

const size_t Client::INVENTORY_SIZE = 10;

const int Client::PLAYER_ACTION_CHANNEL = 0;

Color Client::SAY_COLOR;
Color Client::WHISPER_COLOR;

bool Client::isClient = false;

std::map<std::string, int> Client::_messageCommands;
std::map<int, std::string> Client::_errorMessages;

Client::Client():
_cursorNormal(std::string("Images/Cursors/normal.png"), Color::MAGENTA),
_cursorGather(std::string("Images/Cursors/gather.png"), Color::MAGENTA),
_cursorContainer(std::string("Images/Cursors/container.png"), Color::MAGENTA),
_currentCursor(&_cursorNormal),

_activeRecipe(nullptr),
_recipeList(nullptr),
_detailsPane(nullptr),
_craftingWindow(nullptr),
_inventoryWindow(nullptr),

_actionTimer(0),
_actionLength(0),

_loop(true),
_socket(),

_defaultFont(nullptr),
_defaultFontOffset(0),

_mouse(0,0),
_mouseMoved(false),
_mouseOverWindow(false),
_leftMouseDown(false),
_leftMouseDownEntity(nullptr),
_rightMouseDown(false),
_rightMouseDownEntity(nullptr),

_time(SDL_GetTicks()),
_timeElapsed(0),
_lastPingReply(_time),
_lastPingSent(_time),
_latency(0),
_timeSinceConnectAttempt(CONNECT_RETRY_DELAY),
_fps(0),

_invalidUsername(false),
_loggedIn(false),

_health(0),

_loaded(false),

_timeSinceLocUpdate(0),

_tooltipNeedsRefresh(false),

_mapX(0), _mapY(0),

_currentMouseOverEntity(nullptr),

_confirmationWindow(nullptr),
_confirmationWindowText(nullptr),

_serialToDrop(0),
_slotToDrop(Container::NO_SLOT),

_debug("client.log"){
    isClient = true;
    _instance = this;
    _debugInstance = &_debug;


    // Read config file
    XmlReader xr("client-config.xml");

    px_t
        chatW = 150,
        chatH = 100;
    auto elem = xr.findChild("chatLog");
    xr.findAttr(elem, "width", chatW);
    xr.findAttr(elem, "height", chatH);

    std::string fontFile = "poh_pixels.ttf";
    int fontSize = 16;
    elem = xr.findChild("gameFont");
    xr.findAttr(elem, "filename", fontFile);
    xr.findAttr(elem, "size", fontSize);
    _defaultFont = TTF_OpenFont(fontFile.c_str(), fontSize);
    Element::font(_defaultFont);
    xr.findAttr(elem, "offset", _defaultFontOffset);
    Element::textOffset = _defaultFontOffset;
    xr.findAttr(elem, "height", Element::TEXT_HEIGHT);

    px_t castBarY = 300, castBarW = 150, castBarH = 11;
    elem = xr.findChild("castBar");
    xr.findAttr(elem, "y", castBarY);
    xr.findAttr(elem, "w", castBarW);
    xr.findAttr(elem, "h", castBarH);


    Element::initialize();

    // Initialize chat log
    _chatContainer = new Element(Rect(0, SCREEN_Y - chatH, chatW, chatH));
    _chatTextBox = new TextBox(Rect(0, chatH, chatW));
    _chatLog = new List(Rect(0, 0, chatW, chatH - _chatTextBox->height()));
    _chatTextBox->rect(0, _chatLog->height());
    _chatTextBox->hide();
    _chatContainer->addChild(_chatLog);
    _chatContainer->addChild(_chatTextBox);

    addUI(_chatContainer);
    SAY_COLOR = Color::WHITE;
    WHISPER_COLOR = Color::RED/2 + Color::WHITE/2;

    initializeMessageNames();

    SDL_ShowCursor(SDL_DISABLE);

    _debug << cmdLineArgs << Log::endl;
    Socket::debug = &_debug;

    int ret = (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 512) < 0);
    if (ret < 0){
        _debug("SDL_mixer failed to initialize.", Color::RED);
    } else {
        _debug("SDL_mixer initialized.");
    }

    // Resolve default server IP
    elem = xr.findChild("server");
    std::string serverHostDirectory;
    xr.findAttr(elem, "hostDirectory", serverHostDirectory);
    _defaultServerAddress = readFromURL(serverHostDirectory);

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
        _inventory.push_back(std::make_pair<const Item *, size_t>(nullptr, 0));

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
        if (xr.findAttr(elem, "deconstructs", s)) cot.canDeconstruct(true);
        
        auto container = xr.findChild("container", elem);
        if (container != nullptr) {
            if (xr.findAttr(container, "slots", n)) cot.containerSlots(n);
        }

        if (xr.findAttr(elem, "merchantSlots", n)) cot.merchantSlots(n);

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
    
    // Initialize cast bar
    const Rect
        CAST_BAR_RECT(SCREEN_X/2 - castBarW/2, castBarY, castBarW, castBarH),
        CAST_BAR_DIMENSIONS(0, 0, castBarW, castBarH);
    static const Color CAST_BAR_LABEL_COLOR = Color::BLUE / 2 + Color::GREY_2;
    _castBar = new Element(CAST_BAR_RECT);
    _castBar->addChild(new ProgressBar<ms_t>(CAST_BAR_DIMENSIONS, _actionTimer, _actionLength));
    LinkedLabel<std::string> *castBarLabel = new LinkedLabel<std::string>(CAST_BAR_DIMENSIONS,
                                                                          _actionMessage, "", "",
                                                                          Label::CENTER_JUSTIFIED);
    castBarLabel->setColor(CAST_BAR_LABEL_COLOR);
    _castBar->addChild(castBarLabel);
    _castBar->hide();
    addUI(_castBar);

    // Initialize menu bar
    static const px_t
        MENU_BUTTON_W = 50,
        MENU_BUTTON_H = 13,
        NUM_BUTTONS = 3;
    Element *menuBar = new Element(Rect(SCREEN_X/2 - MENU_BUTTON_W * NUM_BUTTONS / 2,
                                        SCREEN_Y - MENU_BUTTON_H,
                                        MENU_BUTTON_W * NUM_BUTTONS,
                                        MENU_BUTTON_H));
    menuBar->addChild(new Button(Rect(0, 0, MENU_BUTTON_W, MENU_BUTTON_H), "Crafting",
                                 Element::toggleVisibilityOf, _craftingWindow));
    menuBar->addChild(new Button(Rect(MENU_BUTTON_W, 0, MENU_BUTTON_W, MENU_BUTTON_H), "Inventory",
                                 Element::toggleVisibilityOf, _inventoryWindow));
    menuBar->addChild(new Button(Rect(MENU_BUTTON_W * 2, 0, MENU_BUTTON_W, MENU_BUTTON_H), "Chat",
                                 Element::toggleVisibilityOf, _chatContainer));
    addUI(menuBar);

    // Initialize FPS/latency display
    static const px_t
        HARDWARE_STATS_W = 60,
        HARDWARE_STATS_H = 22,
        HARDWARE_STATS_LABEL_HEIGHT = 11;
    static const Rect
        HARDWARE_STATS_RECT(SCREEN_X - HARDWARE_STATS_W, 0, HARDWARE_STATS_W, HARDWARE_STATS_H);
    Element *hardwareStats = new Element(Rect(SCREEN_X - HARDWARE_STATS_W, 0,
                                              HARDWARE_STATS_W, HARDWARE_STATS_H));
    LinkedLabel<unsigned> *fps = new LinkedLabel<unsigned>(
        Rect(0, 0, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _fps, "", "fps", Label::RIGHT_JUSTIFIED);
    LinkedLabel<ms_t> *lat = new LinkedLabel<ms_t>(
        Rect(0, HARDWARE_STATS_LABEL_HEIGHT, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _latency, "", "ms", Label::RIGHT_JUSTIFIED);
    fps->setColor(Color::YELLOW);
    lat->setColor(Color::YELLOW);
    hardwareStats->addChild(fps);
    hardwareStats->addChild(lat);
    addUI(hardwareStats);

    // Initialize health bar
    static const px_t
        HEALTH_BAR_LENGTH = 130,
        HEALTH_BAR_HEIGHT = 13;
    Element *healthBar = new Element(Rect(0, 0, HEALTH_BAR_LENGTH, HEALTH_BAR_HEIGHT));
    static const unsigned MAX_HEALTH = 100;
    healthBar->addChild(new ProgressBar<unsigned>(Rect(0, 0, HEALTH_BAR_LENGTH, HEALTH_BAR_HEIGHT),
                                                  _health, MAX_HEALTH));
    addUI(healthBar);
}

Client::~Client(){
    SDL_ShowCursor(SDL_ENABLE);
    Element::cleanup();
    if (_defaultFont != nullptr)
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
        std::string serverIP;
        if (cmdLineArgs.contains("server-ip"))
            serverIP = cmdLineArgs.getString("server-ip");
        else{
            serverIP = _defaultServerAddress;
        }
        sockaddr_in serverAddr;
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
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
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
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

    ms_t timeAtLastTick = SDL_GetTicks();
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
        _fps = toInt(1000.0 / _timeElapsed);

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

        handleInput(delta);
        
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
    static const px_t LOOKUP_MARGIN = 30;
    Entity
        topEntity(nullptr, Point(0, mouseOffset.y - LOOKUP_MARGIN)),
        bottomEntity(nullptr, Point(0, mouseOffset.y + LOOKUP_MARGIN));
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
        return nullptr;
}

void Client::checkMouseOver(){
    _currentCursor = &_cursorNormal;

    // Check whether mouse is over a window
    _mouseOverWindow = false;
    for (const Window *window : _windows)
        if (window->visible() && collision(_mouse, window->rect())){
            _mouseOverWindow = true;
            _currentMouseOverEntity = nullptr;
            return;
        }

    // Check if mouse is over an entity
    const Entity *const oldMouseOverEntity = _currentMouseOverEntity;
    _currentMouseOverEntity = getEntityAtMouse();
    if (_currentMouseOverEntity == nullptr)
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

void Client::startCrafting(void *data){
    if (_instance->_activeRecipe != nullptr) {
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

void Client::startAction(ms_t actionLength){
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

void Client::addChatMessage(const std::string &msg, const Color &color){
    Label *label = new Label(Rect(), msg);
    label->setColor(color);
    bool atBottom = _chatLog->isScrolledToBottom() || !_chatLog->isScrollBarVisible();
    _chatLog->addChild(label);
    if (atBottom)
        _chatLog->scrollToBottom();
}

void Client::watchObject(ClientObject &obj){
    sendMessage(CL_START_WATCHING, makeArgs(obj.serial()));
    _objectsWatched.insert(&obj);
}

void Client::unwatchObject(ClientObject &obj){
    sendMessage(CL_STOP_WATCHING, makeArgs(obj.serial()));
    _objectsWatched.erase(&obj);
}

void Client::dropItemOnConfirmation(size_t serial, size_t slot, const Item *item){
    _serialToDrop = serial;
    _slotToDrop = slot;
    std::string windowText = "Are you sure you want to drop ";
    windowText += item->name() + "?";

    _messageToConfirm = MSG_START + toString(CL_DROP) + MSG_DELIM + makeArgs(serial, slot) +
                        MSG_END;

    if (_confirmationWindow == nullptr){
        static const px_t
            WINDOW_WIDTH = 200,
            PADDING = 2,
            BUTTON_WIDTH = 60,
            BUTTON_HEIGHT = 15,
            WINDOW_HEIGHT = BUTTON_HEIGHT + 3 * PADDING + Element::TEXT_HEIGHT,
            BUTTON_Y = 2 * PADDING + Element::TEXT_HEIGHT;
        _confirmationWindow = new Window(Rect((SCREEN_X - WINDOW_WIDTH) / 2,
                                              (SCREEN_Y - WINDOW_HEIGHT) / 2,
                                              WINDOW_WIDTH, WINDOW_HEIGHT),
                                         "Confirmation");
        _confirmationWindowText = new Label(Rect(0, PADDING, WINDOW_WIDTH, Element::TEXT_HEIGHT),
                                            windowText, Element::CENTER_JUSTIFIED);
        _confirmationWindow->addChild(_confirmationWindowText);
        _confirmationWindow->addChild(new Button(Rect((WINDOW_WIDTH - PADDING) / 2 - BUTTON_WIDTH,
                                                      BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT),
                                                "OK", sendMessageAndHideConfirmationWindow));
        _confirmationWindow->addChild(new Button(Rect((WINDOW_WIDTH + PADDING) / 2, BUTTON_Y,
                                                       BUTTON_WIDTH, BUTTON_HEIGHT),
                                                 "Cancel",
                                                 Window::hideWindow, _confirmationWindow));
        addWindow(_confirmationWindow);
    } else {
        _confirmationWindowText->changeText(windowText);
    }
    _confirmationWindow->show();
}

void Client::sendMessageAndHideConfirmationWindow(void *data){
    _instance->sendRawMessage(_instance->_messageToConfirm);
    _instance->_confirmationWindow->hide();
}

bool Client::outsideCullRange(const Point &loc, px_t hysteresis) const{
    px_t testCullDist = CULL_DISTANCE + hysteresis;
    return
        abs(loc.x - _character.location().x) > testCullDist ||
        abs(loc.y - _character.location().y) > testCullDist;
}
