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
#include "TooltipBuilder.h"
#include "ui/Button.h"
#include "ui/Container.h"
#include "ui/Element.h"
#include "ui/LinkedLabel.h"
#include "ui/ProgressBar.h"
#include "../XmlReader.h"
#include "../messageCodes.h"
#include "../util.h"
#include "../server/User.h"

extern Args cmdLineArgs;

// TODO: Move all client functionality to a namespace, rather than a class.
Client *Client::_instance = 0;

LogSDL *Client::_debugInstance = 0;

const int Client::SCREEN_X = 640;
const int Client::SCREEN_Y = 360;

const Uint32 Client::MAX_TICK_LENGTH = 100;
const Uint32 Client::SERVER_TIMEOUT = 10000;
const Uint32 Client::CONNECT_RETRY_DELAY = 3000;
const Uint32 Client::PING_FREQUENCY = 5000;

const Uint32 Client::TIME_BETWEEN_LOCATION_UPDATES = 50;

const int Client::ICON_SIZE = 16;
const int Client::HEADING_HEIGHT = 14;
const int Client::LINE_GAP = 6;

const int Client::TILE_W = 32;
const int Client::TILE_H = 32;
const double Client::MOVEMENT_SPEED = 80;

const int Client::ACTION_DISTANCE = 30;

const size_t Client::INVENTORY_SIZE = 10;

const size_t Client::MAX_TEXT_ENTERED = 100;

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
_fps(0),

_invalidUsername(false),
_loggedIn(false),
_loaded(false),

_timeSinceLocUpdate(0),

_tooltipNeedsRefresh(false),

_mapX(0), _mapY(0),

_currentMouseOverEntity(0),

_debug("client.log"){
    isClient = true;
    _instance = this;
    _debugInstance = &_debug;


    // Read config file
    XmlReader xr("client-config.xml");

    int
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

    int castBarY = 300, castBarW = 150, castBarH = 11;
    elem = xr.findChild("castBar");
    xr.findAttr(elem, "y", castBarY);
    xr.findAttr(elem, "w", castBarW);
    xr.findAttr(elem, "h", castBarH);


    Element::initialize();

    // Initialize chat log
    _chatContainer = new Element(Rect(0, SCREEN_Y - chatH, chatW, chatH));
    _chatLog = new List(Rect(1, 1, chatW - 2, chatH - 2));
    _chatContainer->addChild(_chatLog);
    _chatContainer->addChild(new ShadowBox(Rect(0, 0, chatW, chatH), true));
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
        if (container) {
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
    _castBar->addChild(new ProgressBar<Uint32>(CAST_BAR_DIMENSIONS, _actionTimer, _actionLength));
    LinkedLabel<std::string> *castBarLabel = new LinkedLabel<std::string>(CAST_BAR_DIMENSIONS,
                                                                          _actionMessage, "", "",
                                                                          Label::CENTER_JUSTIFIED);
    castBarLabel->setColor(CAST_BAR_LABEL_COLOR);
    _castBar->addChild(castBarLabel);
    _castBar->hide();
    addUI(_castBar);

    // Initialize menu bar
    static const int
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
    static const int
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
    LinkedLabel<Uint32> *lat = new LinkedLabel<Uint32>(
        Rect(0, HARDWARE_STATS_LABEL_HEIGHT, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _latency, "", "ms", Label::RIGHT_JUSTIFIED);
    fps->setColor(Color::YELLOW);
    lat->setColor(Color::YELLOW);
    hardwareStats->addChild(fps);
    hardwareStats->addChild(lat);
    addUI(hardwareStats);
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

void Client::addChatMessage(const std::string &msg, const Color &color){
    Label *label = new Label(Rect(), msg);
    label->setColor(color);
    bool atBottom = _chatLog->isScrolledToBottom() || !_chatLog->isScrollBarVisible();
    _chatLog->addChild(label);
    if (atBottom)
        _chatLog->scrollToBottom();
}
