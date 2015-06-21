#include <cassert>
#include <SDL.h>
#include <string>
#include <sstream>
#include <vector>

#include "Client.h"
#include "EntityType.h"
#include "Renderer.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

extern Args cmdLineArgs;
extern Renderer renderer;

const size_t Client::BUFFER_SIZE = 1023;

const Uint32 Client::MAX_TICK_LENGTH = 100;
const Uint32 Client::SERVER_TIMEOUT = 10000;
const Uint32 Client::CONNECT_RETRY_DELAY = 3000;
const Uint32 Client::PING_FREQUENCY = 5000;

const double Client::MOVEMENT_SPEED = 80;
const Uint32 Client::TIME_BETWEEN_LOCATION_UPDATES = 250;

bool Client::isClient = false;

Client::Client():
_loop(true),
_debug(640/20),
_socket(),
_connected(false),
_invalidUsername(false),
_timeSinceLocUpdate(0),
_locationChanged(false),
_tooltipNeedsRefresh(false),
_character(OtherUser::entityType(), 0),
_inventory(User::INVENTORY_SIZE, std::make_pair("none", 0)),
_time(SDL_GetTicks()),
_timeElapsed(0),
_lastPingSent(_time),
_lastPingReply(_time),
_timeSinceConnectAttempt(CONNECT_RETRY_DELAY),
_loaded(false),
_mouse(0,0),
_currentMouseOverEntity(0){
    isClient = true;

    _debug << cmdLineArgs << Log::endl;
    Socket::debug = &_debug;

    renderer.setDrawColor();

    _entities.insert(&_character);

    _defaultFont = TTF_OpenFont("trebuc.ttf", 16);

    OtherUser::image("Images/man.bmp");
    Branch::image("Images/branch.bmp");

    _invLabel = Texture(_defaultFont, "Inventory");

    // Randomize player name if not supplied
    if (cmdLineArgs.contains("username"))
        _username = cmdLineArgs.getString("username");
    else
        for (int i = 0; i != 3; ++i)
            _username.push_back('a' + rand() % 26);
    _debug << "Player name: " << _username << Log::endl;

    // Load game data
    _items.insert(Item("wood", "wood", 5));
    _items.insert(Item("none", "none"));
}

Client::~Client(){
    if (_defaultFont)
        TTF_CloseFont(_defaultFont);
}

void Client::checkSocket(){
    if (_invalidUsername)
        return;

    // Ensure connected to server
    if (!_connected && _timeSinceConnectAttempt >= CONNECT_RETRY_DELAY) {
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
            _debug << Color::GREEN << "Connected to server" << Log::endl;
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
        if (_connected && _time - _lastPingSent > PING_FREQUENCY) {
            sendMessage(CL_PING, makeArgs(_time));
            _lastPingSent = _time;
        }

        _timeElapsed = _time - timeAtLastTick;
        if (_timeElapsed > MAX_TICK_LENGTH)
            _timeElapsed = MAX_TICK_LENGTH;
        double delta = _timeElapsed / 1000.0;
        timeAtLastTick = _time;

        // Ensure server connectivity
        if (_connected && _time - _lastPingReply > SERVER_TIMEOUT) {
            _debug << Color::RED << "Disconnected from server" << Log::endl;
            _socket = Socket();
            _connected = false;
        }

        if (!_connected) {
            _timeSinceConnectAttempt += _timeElapsed;

        } else { // Update server with current location
            _timeSinceLocUpdate += _timeElapsed;
            if (_locationChanged && _timeSinceLocUpdate > TIME_BETWEEN_LOCATION_UPDATES) {
                sendMessage(CL_LOCATION, makeArgs(_character.location().x, _character.location().y));
                _locationChanged = false;
                _timeSinceLocUpdate = 0;
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

            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    _loop = false;
                    break;
                }
                break;

            case SDL_MOUSEMOTION: {
                _mouse.x = e.motion.x;
                _mouse.y = e.motion.y;

                if (!_loaded)
                    break;

                // Check if mouse is over an entity
                const Entity *oldMouseOverEntity = _currentMouseOverEntity;
                Entity::set_t::iterator mouseOverIt = _entities.end();
                static EntityType dummyEntityType(makeRect());
                Entity lookupEntity(dummyEntityType, _mouse);
                for (Entity::set_t::iterator it = _entities.lower_bound(&lookupEntity); it != _entities.end(); ++it) {
                    if ((*it)->collision(_mouse))
                        mouseOverIt = it;
                }
                if (mouseOverIt != _entities.end()) {
                    _currentMouseOverEntity = *mouseOverIt;
                    if (_currentMouseOverEntity != oldMouseOverEntity ||
                        _currentMouseOverEntity->needsTooltipRefresh() ||
                        _tooltipNeedsRefresh ||
                        _locationChanged) {
                        _currentMouseOverEntity->refreshTooltip(*this);
                        _tooltipNeedsRefresh = false;
                    }
                        
                } else
                    _currentMouseOverEntity = 0;

                break;
            }

            case SDL_MOUSEBUTTONUP:
                if (!_loaded)
                    break;

                if (_currentMouseOverEntity)
                    _currentMouseOverEntity->onLeftClick(*this);

                break;

            default:
                // Unhandled event
                ;
            }
        }
        // Poll keys (whether they are currently pressed; not key events)
        static const Uint8 *keyboardState = SDL_GetKeyboardState(0);
        if (_connected) {
            bool
                up = keyboardState[SDL_SCANCODE_UP] == SDL_PRESSED,
                down = keyboardState[SDL_SCANCODE_DOWN] == SDL_PRESSED,
                left = keyboardState[SDL_SCANCODE_LEFT] == SDL_PRESSED,
                right = keyboardState[SDL_SCANCODE_RIGHT] == SDL_PRESSED;
            if (up != down || left != right) {
                static const double DIAG_SPEED = MOVEMENT_SPEED / SQRT_2;
                double
                    dist = delta * MOVEMENT_SPEED,
                    diagDist = delta * DIAG_SPEED;
                Point newLoc = _character.location();
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
                setEntityLocation(&_character, newLoc);
                _locationChanged = true;
            }
        }

        // Update entities
        std::vector<Entity *> entitiesToReorder;
        for (Entity::set_t::iterator it = _entities.begin(); it != _entities.end(); ) {
            Entity::set_t::iterator next = it; ++next;
            Entity *toUpdate = *it;
            toUpdate->update(delta);
            if (toUpdate->yChanged()) {
                // Entity has moved up or down, and must be re-ordered in set.
                entitiesToReorder.push_back(toUpdate);
                _entities.erase(it);
                toUpdate->yChanged(false);
            }
            it = next;
        }
        for (std::vector<Entity *>::iterator it = entitiesToReorder.begin(); it != entitiesToReorder.end(); ++it)
            _entities.insert(*it);
        entitiesToReorder.clear();

        checkSocket();
        // Draw
        draw();
        SDL_Delay(10);
    }
}

void Client::draw(){
    if (!_connected || !_loaded){
        renderer.setDrawColor(Color::BLACK);
        renderer.clear();
        _debug.draw();
        renderer.present();
        return;
    }

    // Background
    static const Color backgroundColor = Color::GREEN/4;
    renderer.setDrawColor(Color::GREEN / 4);
    renderer.clear();

    // Entities, sorted from back to front
    for (Entity::set_t::const_iterator it = _entities.begin(); it != _entities.end(); ++it)
        (*it)->draw(*this);

    // Rectangle around user
    renderer.setDrawColor(Color::WHITE);
    SDL_Rect drawLoc = _character.drawRect();
    renderer.drawRect(_character.drawRect());

    // Inventory
    static SDL_Rect invBackgroundRect = makeRect(renderer.width() - 250, renderer.height() - 70, 250, 60);
    renderer.setDrawColor(Color::WHITE / 4);
    renderer.fillRect(invBackgroundRect);
    _invLabel.draw(invBackgroundRect.x, invBackgroundRect.y);
    renderer.setDrawColor(Color::BLACK);
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        SDL_Rect iconRect = makeRect(renderer.width() - 248 + i*50, renderer.height() - 48, 48, 48);
        renderer.fillRect(iconRect);
        std::set<Item>::iterator it = _items.find(_inventory[i].first);
        if (it == _items.end())
            _debug << Color::RED << "Unknown item: " << _inventory[i].first;
        else {
            it->icon().draw(iconRect);
            if (it->stackSize() > 1) {
                // Display stack size
                Texture qtyLabel(_defaultFont, makeArgs(makeArgs(_inventory[i].second)));
                qtyLabel.draw(iconRect.x + 48 - qtyLabel.width(), iconRect.y + 48 - qtyLabel.height());
            }
        }
    }

    // Tooltip
    if (_currentMouseOverEntity) {
        Texture tooltip = _currentMouseOverEntity->tooltip();
        tooltip.draw(10, renderer.height() - tooltip.height() - 10);
    }

    // FPS/latency
    std::ostringstream oss;
    if (_timeElapsed > 0)
        oss << static_cast<int>(1000.0/_timeElapsed + .5);
    else
        oss << "infinite ";
    oss << "fps " << _latency << "ms";
    Texture statsDisplay(_defaultFont, oss.str(), Color::YELLOW);
    statsDisplay.draw(renderer.width() - statsDisplay.width(), 0);

    _debug.draw();
    renderer.present();
}

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
            _debug << Color::RED << "Malformed message; discarded \"" << buffer << "\"" << Log::endl;
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
            int charsRead = iss.gcount();
            buffer[charsRead] = ']';
            buffer[charsRead+1] = '\0';
            iss.ignore(); // Throw away ']'
        }
        std::istringstream singleMsg(buffer);
        singleMsg >> del >> msgCode >> del;
        switch(msgCode) {

        case SV_WELCOME:
        {
            if (del != ']')
                break;
            _connected = true;
            _timeSinceConnectAttempt = 0;
            _lastPingSent = _lastPingReply = _time;
            _debug << Color::GREEN << "Successfully logged in to server" << Log::endl;
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
            std::map<std::string, OtherUser*>::iterator it = _otherUsers.find(name);
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
            _debug << Color::RED << "The user " << _username << " is already connected to the server." << Log::endl;
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
            _debug << Color::YELLOW << "The server is full.  Attempting reconnection..." << Log::endl;
            _socket = Socket();
            _connected = false;
            break;

        case SV_TOO_FAR:
            if (del != ']')
                break;
            _debug << Color::YELLOW << "You are too far away to perform that action." << Log::endl;
            break;

        case SV_DOESNT_EXIST:
            if (del != ']')
                break;
            _debug << Color::YELLOW << "That object doesn't exist." << Log::endl;
            break;

        case SV_INVENTORY_FULL:
            if (del != ']')
                break;
            _debug << Color::YELLOW << "Your inventory is full." << Log::endl;
            break;

        case SV_LOCATION:
        {
            std::string name;
            double x, y;
            singleMsg.get(buffer, BUFFER_SIZE, ',');
            name = std::string(buffer);
            singleMsg >> del >> x >> del >> y >> del;
            if (del != ']')
                break;
            Point p(x, y);
            if (name == _username) {
                setEntityLocation(&_character, p);
                _loaded = true;
                _tooltipNeedsRefresh = true;
            } else {
                if (_otherUsers.find(name) == _otherUsers.end()) {
                    // Create new OtherUser
                    OtherUser *newUser = new OtherUser(name, p);
                    _otherUsers[name] = newUser;
                    _entities.insert(newUser);
                }
                _otherUsers[name]->destination(p);
            }
            break;
        }

        case SV_INVENTORY:
        {
            int slot, quantity;
            std::string itemID;
            singleMsg >> slot >> del;
            singleMsg.get(buffer, BUFFER_SIZE, ',');
            itemID = std::string(buffer);
            singleMsg >> del >> quantity >> del;
            if (del != ']')
                break;
            _inventory[slot] = std::make_pair(itemID, quantity);
            break;
        }

        case SV_BRANCH:
        {
            int serial;
            double x, y;
            singleMsg >> serial >> del >> x >> del >> y >> del;
            if (del != ']')
                break;
            std::map<size_t, Branch*>::iterator it = _branches.find(serial);
            if (it == _branches.end()) {
                // A new branch was added; add entity to list
                Branch *newBranch = new Branch(serial, Point(x, y));
                _entities.insert(newBranch);
                _branches[serial] = newBranch;
            }
            break;
        }

        case SV_REMOVE_BRANCH:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != ']')
                break;
            std::map<size_t, Branch*>::const_iterator it = _branches.find(serial);
            if (it == _branches.end()){
                _debug << Color::YELLOW << "Server removed a branch we didn't know about." << Log::endl;
                assert(false);
                break; // We didn't know about this branch
            }
            if (it->second == _currentMouseOverEntity)
                _currentMouseOverEntity = 0;
            removeEntity(it->second);
            _branches.erase(it);
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }

        if (del != ']' && !iss.eof()) {
            _debug << Color::RED << "Bad message ending" << Log::endl;
        }

        iss.peek();
    }
}

void Client::sendMessage(MessageCode msgCode, const std::string &args) const{
    // Compile message
    std::ostringstream oss;
    oss << '[' << msgCode;
    if (args != "")
        oss << ',' << args;
    oss << ']';

    // Send message
    _socket.sendMessage(oss.str());
}

const Socket &Client::socket() const{
    return _socket;
}

void Client::removeEntity(Entity *const toRemove){
    Entity::set_t::iterator it = _entities.find(toRemove);
    if (it != _entities.end())
        _entities.erase(it);
    delete toRemove;
}

TTF_Font *Client::defaultFont() const{
    return _defaultFont;
}

void Client::setEntityLocation(Entity *entity, const Point &location){
    Entity::set_t::iterator it = _entities.find(entity);
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
