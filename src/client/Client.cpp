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
#include "SpriteType.h"
#include "ClientCombatant.h"
#include "LogSDL.h"
#include "Particle.h"
#include "Tooltip.h"
#include "ui/Button.h"
#include "ui/CombatantPanel.h"
#include "ui/ConfirmationWindow.h"
#include "ui/ContainerGrid.h"
#include "ui/Element.h"
#include "ui/LinkedLabel.h"
#include "ui/ProgressBar.h"
#include "../XmlReader.h"
#include "../curlUtil.h"
#include "../messageCodes.h"
#include "../util.h"
#include "../versionUtil.h"
#include "../server/User.h"

extern Args cmdLineArgs;

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
const double Client::VEHICLE_SPEED = 20;
const Hitpoints Client::MAX_PLAYER_HEALTH = 50;

const px_t Client::ACTION_DISTANCE = 30;

const px_t Client::CULL_DISTANCE = 450; // Just copy Server::CULL_DISTANCE
const px_t Client::CULL_HYSTERESIS_DISTANCE = 50;

const size_t Client::INVENTORY_SIZE = 10;
const size_t Client::GEAR_SLOTS = 8;
std::vector<std::string> Client::GEAR_SLOT_NAMES;

Color Client::SAY_COLOR;
Color Client::WHISPER_COLOR;

bool Client::isClient = false;

std::map<std::string, int> Client::_messageCommands;
std::map<int, std::string> Client::_errorMessages;

const int Client::MIXING_CHANNELS = 32;

Client::Client():
_cursorNormal(std::string("Images/Cursors/normal.png"), Color::MAGENTA),
_cursorGather(std::string("Images/Cursors/gather.png"), Color::MAGENTA),
_cursorContainer(std::string("Images/Cursors/container.png"), Color::MAGENTA),
_cursorAttack(std::string("Images/Cursors/attack.png"), Color::MAGENTA),
_currentCursor(&_cursorNormal),

_character({}, {}),
_isDismounting(false),

_activeRecipe(nullptr),

_selectedConstruction(nullptr),
_multiBuild(false),

_connectionStatus(INITIALIZING),

_actionTimer(0),
_actionLength(0),

_loop(true),
_running(false),
_freeze(false),
_socket(),
_dataLoaded(false),

_defaultFont(nullptr),

_mouse(0,0),
_mouseMoved(false),
_mouseOverWindow(false),
_leftMouseDown(false),
_leftMouseDownEntity(nullptr),
_rightMouseDown(false),
_rightMouseDownEntity(nullptr),
_rightMouseDownWasOnUI(false),

_basePassive(std::string("Images/targetPassive.png"), Color::MAGENTA),
_baseAggressive(std::string("Images/targetAggressive.png"), Color::MAGENTA),
_inventory(INVENTORY_SIZE, std::make_pair(nullptr, 0)),

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

_currentMouseOverEntity(nullptr),

_confirmDropItem(nullptr),

_drawingFinished(false),

_avatarSounds(nullptr),
_channelsPlaying(0),

_numEntities(0),

_debug("client.log"){
    _defaultFont = TTF_OpenFont("AdvoCut.ttf", 10);
    drawLoadingScreen("Reading configuration file", 0.1);

    isClient = true;
    _instance = this;
    _debugInstance = &_debug;

    _config.loadFromFile("client-config.xml");
    
    initUI();

    initializeMessageNames();

    SDL_ShowCursor(SDL_DISABLE);

#ifdef _DEBUG
    _debug << Color::FONT << cmdLineArgs << Log::endl;
    if (Socket::debug == nullptr)
        Socket::debug = &_debug;
#endif
    
    drawLoadingScreen("Initializing audio", 0.5);
    int ret = (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, 512) < 0);
    if (ret < 0){
        _debug("SDL_mixer failed to initialize.", Color::FAILURE);
    }
    Mix_AllocateChannels(MIXING_CHANNELS);

    // Resolve default server IP
    drawLoadingScreen("Finding server", 0.7);
    if (!cmdLineArgs.contains("server-ip")){
        _defaultServerAddress = readFromURL(_config.serverHostDirectory);
    }

    renderer.setDrawColor();

    _entities.insert(&_character);

    initializeUsername();

    SDL_StopTextInput();

    drawLoadingScreen("", 1);
}

void Client::initializeGearSlotNames(){
    if (! GEAR_SLOT_NAMES.empty())
        return;
    GEAR_SLOT_NAMES.push_back("Head");
    GEAR_SLOT_NAMES.push_back("Jewelry");
    GEAR_SLOT_NAMES.push_back("Body");
    GEAR_SLOT_NAMES.push_back("Shoulders");
    GEAR_SLOT_NAMES.push_back("Hands");
    GEAR_SLOT_NAMES.push_back("Feet");
    GEAR_SLOT_NAMES.push_back("Right hand");
    GEAR_SLOT_NAMES.push_back("Left hand");
}

Client::~Client() {
    SDL_ShowCursor(SDL_ENABLE);
    cleanUpLoginScreen();
    Element::cleanup();
    if (_defaultFont != nullptr)
        TTF_CloseFont(_defaultFont);
    for (const Sprite *entityConst : _entities) {
        Sprite *entity = const_cast<Sprite *>(entityConst);
        if (entity == &_character)
            continue;
        delete entity;
    }

    for (auto type : _objectTypes)
        delete type;
    for (auto profile : _particleProfiles)
        delete profile;
    for (auto projectileType : _projectileTypes)
        delete projectileType;
    for (const auto &spellPair : _spells)
        delete spellPair.second;

    // Some entities will destroy their own windows, and remove them from this list.
    for (Window *window : _windows)
        delete window;

    cleanupSocialWindow();

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
            _debug << Color::FAILURE << "Connection error: " << WSAGetLastError() << Log::endl;
            _connectionStatus = CONNECTION_ERROR;
        } else {
#ifdef _DEBUG
            _debug("Connected to server", Color::SUCCESS);
#endif
            // Announce player name
            sendMessage(CL_I_AM, makeArgs(_username, version()));
            sendMessage(CL_PING, makeArgs(SDL_GetTicks()));
            _connectionStatus = CONNECTED;
            _character.name(_username);
        }
    }

    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::FAILURE << "Error polling sockets: " << WSAGetLastError() << Log::endl;
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
    _running = true;
    if (!_dataLoaded){
        drawLoadingScreen("Loading data", 0.6);
        bool shouldLoadDefaultData = true;
        if (cmdLineArgs.contains("load-test-data-first")){
            loadData("testing/data/minimal");
            shouldLoadDefaultData = false;
        }
        if (cmdLineArgs.contains("data")){
            loadData(cmdLineArgs.getString("data"));
            shouldLoadDefaultData = false;
        }
        if (shouldLoadDefaultData)
            loadData();
    }

    populateHotbar();
    
    drawLoadingScreen("Initializing login screen", 0.9);
    initLoginScreen();
    _connectionStatus = IN_LOGIN_SCREEN;

    ms_t timeAtLastTick = SDL_GetTicks();
    while (_loop) {
        _time = SDL_GetTicks();

        _timeElapsed = _time - timeAtLastTick;
        if (_timeElapsed > MAX_TICK_LENGTH)
            _timeElapsed = MAX_TICK_LENGTH;
        timeAtLastTick = _time;
        _fps = toInt(1000.0 / _timeElapsed);

        if (_loggedIn)
            gameLoop();
        else
            loginScreenLoop();

        while (_freeze)
            ;
    }
    _running = false;
}

void Client::gameLoop(){
    const double delta = _timeElapsed / 1000.0; // Fraction of a second that has elapsed

    // Send ping
    if (_time - _lastPingSent > PING_FREQUENCY) {
        sendMessage(CL_PING, makeArgs(_time));
        _lastPingSent = _time;
    }

    // Ensure server connectivity
    if (_time - _lastPingReply > SERVER_TIMEOUT) {
        _debug("Disconnected from server", Color::FAILURE);
        _socket = Socket();
        _loggedIn = false;
    }

    // Update server with current location
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

    // Deal with any messages from the server
    if (!_messages.empty()){
        handleMessage(_messages.front());
        _messages.pop();
    }

    handleInput(delta);
        
    // Update entities
    std::vector<Sprite *> entitiesToReorder;
    for (Sprite::set_t::iterator it = _entities.begin(); it != _entities.end(); ) {
        Sprite::set_t::iterator next = it;
        ++next;
        Sprite *const toUpdate = *it;
        if (toUpdate->markedForRemoval()){
            _entities.erase(it);
            it = next;
            continue;
        }
        toUpdate->update(delta);
        if (toUpdate->yChanged()) {
            // Sprite has moved up or down, and must be re-ordered in set.
            entitiesToReorder.push_back(toUpdate);
            _entities.erase(it);
            toUpdate->yChanged(false);
        }
        it = next;
    }
    for (Sprite *entity : entitiesToReorder)
        _entities.insert(entity);
    entitiesToReorder.clear();

    updateOffset();

    // Update cast bar
    if (_actionLength > 0) {
        _actionTimer = min(_actionTimer + _timeElapsed, _actionLength);
        _castBar->show();
    }

    _channelsPlaying = Mix_Playing(-1);
    _numEntities = _entities.size();

    // Update terrain animation
    for (auto &terrainPair : _terrain)
        terrainPair.second.advanceTime(_timeElapsed);

    if (_mouseMoved)
        checkMouseOver();

    checkSocket();
    // Draw
    draw();
    SDL_Delay(5);
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

//bool Client::playerHasTool(const std::string &tagName) const{
//    // Check inventory
//    for (std::pair<const Item *, size_t> slot : _inventory)
//        if slot.first->isTag(tagName)
//            return true;
//
//    //Check nearby objects
//    for (
//    return false;
//}

const Socket &Client::socket() const{
    return _socket;
}

void Client::removeEntity(Sprite *const toRemove){
    const Sprite::set_t::iterator it = _entities.find(toRemove);
    if (it != _entities.end())
        _entities.erase(it);
    delete toRemove;
}

TTF_Font *Client::defaultFont() const{
    return _defaultFont;
}

void Client::setEntityLocation(Sprite *entity, const MapPoint &location){
    const Sprite::set_t::iterator it = _entities.find(entity);
    if (it == _entities.end()){
        assert(false); // Sprite is not in set.
        return;
    }
    entity->location(location);
    if (entity->yChanged()) {
        _entities.erase(it);
        _entities.insert(entity);
        entity->yChanged(false);
    }
}

void Client::updateOffset() {
    _offset = { SCREEN_X / 2 - _character.location().x,
        SCREEN_Y / 2 - _character.location().y };
    _intOffset = {toInt(_offset.x),
        toInt(_offset.y)};
}

void Client::prepareAction(const std::string &msg){
    _actionMessage = msg;
}

void Client::startAction(ms_t actionLength){
    _actionTimer = 0;
    _actionLength = actionLength;
    if (actionLength == 0)
        _castBar->hide();
}

void Client::addWindow(Window *window){
    assert(window != nullptr);
    assert(!window->title().empty());
    _windows.push_front(window);
}

void Client::removeWindow(Window *window){
    _windows.remove(window);
}

void Client::addUI(Element *element){
    _ui.push_back(element);
}

void Client::addChatMessage(const std::string &msg, const Color &color){
    Label *label = new Label({}, msg);
    label->setColor(color);
    _chatLog->addChild(label);
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
    std::string windowText = "Are you sure you want to destroy " + item->name() + "?";
    std::string msgArgs = makeArgs(serial, slot);

    if (_confirmDropItem != nullptr){
        removeWindow(_confirmDropItem);
        delete _confirmDropItem;
    }
    _confirmDropItem = new ConfirmationWindow(windowText, CL_DROP, msgArgs);
    addWindow(_confirmDropItem);
    _confirmDropItem->show();
}

bool Client::outsideCullRange(const MapPoint &loc, px_t hysteresis) const{
    px_t testCullDist = CULL_DISTANCE + hysteresis;
    return
        abs(loc.x - _character.location().x) > testCullDist ||
        abs(loc.y - _character.location().y) > testCullDist;
}

const ParticleProfile *Client::findParticleProfile(const std::string &id){
    ParticleProfile dummy(id);
    auto it = _particleProfiles.find(&dummy);
    if (it == _particleProfiles.end())
        return nullptr;
    return *it;
}

void Client::addParticles(const ParticleProfile *profile, const MapPoint &location, size_t qty){
#ifdef _DEBUG
    return;
#endif

    for (size_t i = 0; i != qty; ++i) {
        Particle *particle = profile->instantiate(location);
        addEntity(particle);
    }
}

void Client::addParticles(const ParticleProfile *profile, const MapPoint &location){
    if (profile == nullptr)
        return;
    size_t qty = profile->numParticlesDiscrete();
    addParticles(profile, location, qty);
}

void Client::addParticles(const ParticleProfile *profile, const MapPoint &location, double delta){
    if (profile == nullptr)
        return;
    size_t qty = profile->numParticlesContinuous(delta);
    addParticles(profile, location, qty);
}

void Client::addParticles(const std::string &profileName, const MapPoint &location){
    const ParticleProfile *profile = findParticleProfile(profileName);
    addParticles(profile, location);
}

void Client::addParticles(const std::string &profileName, const MapPoint &location, double delta){
    const ParticleProfile *profile = findParticleProfile(profileName);
    addParticles(profile, location, delta);
}

void Client::initializeUsername(){
    if (cmdLineArgs.contains("username"))
        _username = cmdLineArgs.getString("username");
    else if (isDebug())
        setRandomUsername();
    else
        _username = "";
}

void Client::setRandomUsername(){
    _username.clear();
    for (int i = 0; i != 3; ++i)
        _username.push_back('a' + rand() % 26);
}

const SoundProfile *Client::findSoundProfile(const std::string &id) const{
    auto it = _soundProfiles.find(SoundProfile(id));
    if (it == _soundProfiles.end())
        return nullptr;
    return &*it;
}

bool Client::isAtWarWith(const std::string &username) const{
    // Cities
    if (isAtWarWithCityDirectly(username))
        return true;

    // Players
    std::string cityName = "";
    auto it = _userCities.find(username);
    if (it != _userCities.end())
        cityName = it->second;

    if (_character.cityName().empty()) {
        if (!cityName.empty())
            return isAtWarWithCityDirectly(cityName);
        else
            return isAtWarWithPlayerDirectly(username);
    }
    if (!cityName.empty())
        return isCityAtWarWithCityDirectly(cityName);
    else
        return isCityAtWarWithPlayerDirectly(username);
}

bool Client::isAtWarWith(const Avatar &user) const{
    return isAtWarWith(user.name());
}

void Client::addUser(const std::string &name, const MapPoint &location) {
    auto pUser = new Avatar(name, location);
    _otherUsers[name] = pUser;
    _entities.insert(pUser);
}

void Client::infoWindow(const std::string & text) {
    auto window = new InfoWindow(text);
    addWindow(window);
    window->show();
}

void Client::onSpellHit(const MapPoint &location, const void *data) {
    auto &spell = *reinterpret_cast<const ClientSpell *>(data);
    if (spell.impactParticles())
        instance().addParticles(spell.impactParticles(), location);

    if (spell.sounds())
        spell.sounds()->playOnce("impact"s);
}
