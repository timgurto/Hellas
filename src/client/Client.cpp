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
#include "ClientNPC.h"
#include "EntityType.h"
#include "LogSDL.h"
#include "Particle.h"
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

const px_t Client::CULL_DISTANCE = 320;
const px_t Client::CULL_HYSTERESIS_DISTANCE = 50;

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
_cursorAttack(std::string("Images/Cursors/attack.png"), Color::MAGENTA),
_currentCursor(&_cursorNormal),

_activeRecipe(nullptr),
_recipeList(nullptr),
_detailsPane(nullptr),
_craftingWindow(nullptr),
_inventoryWindow(nullptr),

_connectionStatus(TRYING),

_actionTimer(0),
_actionLength(0),

_loop(true),
_running(false),
_socket(),
_dataLoaded(false),

_defaultFont(nullptr),
_defaultFontOffset(0),

_mouse(0,0),
_mouseMoved(false),
_mouseOverWindow(false),
_leftMouseDown(false),
_leftMouseDownEntity(nullptr),
_rightMouseDown(false),
_rightMouseDownEntity(nullptr),

_targetNPC(nullptr),
_targetNPCName(""),
_targetNPCHealth(0),
_targetNPCMaxHealth(0),
_targetDisplay(nullptr),
_usernameDisplay(nullptr),
_aggressive(false),
_basePassive(std::string("Images/targetPassive.png"), Color::MAGENTA),
_baseAggressive(std::string("Images/targetAggressive.png"), Color::MAGENTA),

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

_drawingFinished(false),

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
    _chatContainer->addChild(new ColorBlock(_chatLog->rect(), Color::MMO_OUTLINE));
    _chatContainer->addChild(_chatLog);
    _chatContainer->addChild(_chatTextBox);

    addUI(_chatContainer);
    SAY_COLOR = Color::MMO_L_GREY;
    WHISPER_COLOR = Color::MMO_PURPLE;

    initializeMessageNames();

    SDL_ShowCursor(SDL_DISABLE);

    _debug << cmdLineArgs << Log::endl;
    if (Socket::debug == nullptr)
        Socket::debug = &_debug;

    int ret = (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 512) < 0);
    if (ret < 0){
        _debug("SDL_mixer failed to initialize.", Color::MMO_RED);
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

    // Player's inventory
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory.push_back(std::make_pair<const ClientItem *, size_t>(nullptr, 0));

    // Randomize player name if not supplied
    if (cmdLineArgs.contains("username"))
        _username = cmdLineArgs.getString("username");
    else
        for (int i = 0; i != 3; ++i)
            _username.push_back('a' + rand() % 26);
    _debug << "Player name: " << _username << Log::endl;
    _character.name(_username);

    SDL_StopTextInput();

    Element::absMouse = &_mouse;

    initializeCraftingWindow();
    initializeInventoryWindow();
    addWindow(_craftingWindow);
    addWindow(_inventoryWindow);
    
    // Initialize cast bar
    const Rect
        CAST_BAR_RECT(SCREEN_X/2 - castBarW/2, castBarY, castBarW, castBarH),
        CAST_BAR_DIMENSIONS(0, 0, castBarW, castBarH);
    static const Color CAST_BAR_LABEL_COLOR = Color::MMO_L_GREY;
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
    fps->setColor(Color::MMO_HIGHLIGHT);
    lat->setColor(Color::MMO_HIGHLIGHT);
    hardwareStats->addChild(fps);
    hardwareStats->addChild(lat);
    addUI(hardwareStats);

    // Initialize player display
    static const px_t
        PLAYER_X = 2,
        PLAYER_Y = 2,
        PLAYER_W = 60,
        PLAYER_H = 23,
        BAR_HEIGHT = 7;
    Element *playerDisplay = new Element(Rect(PLAYER_X, PLAYER_Y, PLAYER_W, PLAYER_H));
    playerDisplay->addChild(new ColorBlock(Rect(0, 0, PLAYER_W, PLAYER_H)));
    playerDisplay->addChild(new ShadowBox(Rect(0, 0, PLAYER_W, PLAYER_H)));
    _usernameDisplay = new Label(Rect(2, 1, PLAYER_W - 4, Element::TEXT_HEIGHT), "",
                                 Element::CENTER_JUSTIFIED);
    playerDisplay->addChild(_usernameDisplay);
    static const unsigned MAX_HEALTH = 100;
    playerDisplay->addChild(new ProgressBar<health_t>(
            Rect(2, PLAYER_H - BAR_HEIGHT - 2, PLAYER_W - 4, BAR_HEIGHT), _health, MAX_HEALTH,
            Color::MMO_L_GREEN));
    addUI(playerDisplay);
    
    // Initialize target display
    static const px_t
        TARGET_X = PLAYER_X * 2 + PLAYER_W,
        TARGET_Y = PLAYER_Y,
        TARGET_W = PLAYER_W,
        TARGET_H = PLAYER_H;
    _targetDisplay = new Element(Rect(TARGET_X, TARGET_Y, TARGET_W, TARGET_H));
    _targetDisplay->addChild(new ColorBlock(Rect(0, 0, TARGET_W, TARGET_H)));
    _targetDisplay->addChild(new ShadowBox(Rect(0, 0, TARGET_W, TARGET_H)));
    _targetDisplay->addChild(new LinkedLabel<std::string>(
            Rect(2, 1, TARGET_W - 4, Element::TEXT_HEIGHT), _targetNPCName, "", "",
            Element::CENTER_JUSTIFIED));
    _targetDisplay->addChild(new ProgressBar<health_t>(
            Rect(2, TARGET_H - BAR_HEIGHT - 2, TARGET_W - 4, BAR_HEIGHT),
            _targetNPCHealth, _targetNPCMaxHealth, Color::MMO_L_GREEN));
    _targetDisplay->hide();
    addUI(_targetDisplay);
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

    for (const ClientObjectType *type : _objectTypes)
        delete type;
    for (const ParticleProfile *profile : _particleProfiles)
        delete profile;

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
            _debug << Color::MMO_RED << "Connection error: " << WSAGetLastError() << Log::endl;
            _connectionStatus = CONNECTION_ERROR;
        } else {
            _debug("Connected to server", Color::MMO_L_GREEN);
            // Announce player name
            sendMessage(CL_I_AM, _username);
            sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
            _connectionStatus = CONNECTED;
            _usernameDisplay->changeText(_username);
        }
    }

    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::MMO_RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
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
    if (!_dataLoaded)
        loadData();

    _running = true;

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
        const double delta = _timeElapsed / 1000.0; // Fraction of a second that has elapsed
        timeAtLastTick = _time;
        _fps = toInt(1000.0 / _timeElapsed);

        // Ensure server connectivity
        if (_loggedIn && _time - _lastPingReply > SERVER_TIMEOUT) {
            _debug("Disconnected from server", Color::MMO_RED);
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
            if (toUpdate->markedForRemoval()){
                _entities.erase(it);
                it = next;
                continue;
            }
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
    _running  = false;
}

void Client::startCrafting(void *data){
    if (_instance->_activeRecipe != nullptr) {
        _instance->sendMessage(CL_CRAFT, _instance->_activeRecipe->id());
        const ClientItem *product = toClientItem(_instance->_activeRecipe->product());
        _instance->prepareAction("Crafting " + product->name());
    }
}

bool Client::playerHasItem(const Item *item, size_t quantity) const{
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        const std::pair<const ClientItem *, size_t> slot = _inventory[i];
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

void Client::dropItemOnConfirmation(size_t serial, size_t slot, const ClientItem *item){
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

void Client::targetNPC(const ClientNPC *npc, bool aggressive){
    bool tellServer = false;
    size_t serialToSend = 0;

    if (npc != nullptr && npc->health() == 0)
        aggressive = false;

    // Same target
    if (npc == _targetNPC){
        if (aggressive != _aggressive){
            assert(npc != nullptr);
            tellServer = true;
            if (aggressive) // Switching from passive to aggressive
                serialToSend = npc->serial();
        }
    }

    // Was aggressive, but switched
    else if (_aggressive){
        tellServer = true;
        if (npc != nullptr && aggressive)
            serialToSend = npc->serial();
    }

    // New target, aggressive
    else if (aggressive){
        assert(npc != nullptr);
        tellServer = true;
        serialToSend = npc->serial();
    }

    _targetNPC = npc;
    _aggressive = aggressive;

    if (tellServer){
        sendMessage(CL_TARGET, makeArgs(serialToSend));
    }

    if (npc == nullptr){
        _targetDisplay->hide();
    } else {
        _targetDisplay->show();
        _targetNPCName = npc->npcType()->name();
        _targetNPCHealth = npc->health();
        _targetNPCMaxHealth = npc->npcType()->maxHealth();
    }
}

const ParticleProfile *Client::findParticleProfile(const std::string &id){
    ParticleProfile dummy(id);
    auto it = _particleProfiles.find(&dummy);
    if (it == _particleProfiles.end())
        return nullptr;
    return *it;
}

void Client::addParticles(const ParticleProfile *profile, const Point &location, size_t qty){
    for (size_t i = 0; i != qty; ++i) {
        Particle *particle = profile->instantiate(location);
        addEntity(particle);
    }
}

void Client::addParticles(const ParticleProfile *profile, const Point &location){
    if (profile == nullptr)
        return;
    size_t qty = profile->numParticlesDiscrete();
    addParticles(profile, location, qty);
}

void Client::addParticles(const ParticleProfile *profile, const Point &location, double delta){
    if (profile == nullptr)
        return;
    size_t qty = profile->numParticlesContinuous(delta);
    addParticles(profile, location, qty);
}

void Client::addParticles(const std::string &profileName, const Point &location){
    const ParticleProfile *profile = findParticleProfile(profileName);
    addParticles(profile, location);
}

void Client::addParticles(const std::string &profileName, const Point &location, double delta){
    const ParticleProfile *profile = findParticleProfile(profileName);
    addParticles(profile, location, delta);
}
