#include <cassert>
#include <SDL.h>
#include <string>
#include <sstream>

#include "Client.h"
#include "EntityType.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

const size_t Client::BUFFER_SIZE = 1023;
const int Client::SCREEN_WIDTH = 640;
const int Client::SCREEN_HEIGHT = 480;

const Uint32 Client::MAX_TICK_LENGTH = 100;
const Uint32 Client::SERVER_TIMEOUT = 10000;
const Uint32 Client::CONNECT_RETRY_DELAY = 3000;
const Uint32 Client::PING_FREQUENCY = 5000;

const double Client::MOVEMENT_SPEED = 80;
const Uint32 Client::TIME_BETWEEN_LOCATION_UPDATES = 250;

bool Client::isClient = false;

SDL_Renderer *Client::screen = 0;

Client::Client(const Args &args):
_args(args),
_loop(true),
_debug(SCREEN_HEIGHT/20),
_socket(),
_connected(false),
_invalidUsername(false),
_timeSinceLocUpdate(0),
_locationChanged(false),
_character(OtherUser::entityType(), 0),
_inventory(User::INVENTORY_SIZE, std::make_pair("none", 0)),
_time(SDL_GetTicks()),
_timeElapsed(0),
_lastPingSent(_time),
_lastPingReply(_time),
_timeSinceConnectAttempt(CONNECT_RETRY_DELAY),
_loaded(false),
_mouse(0,0){
    isClient = true;

    _debug << args << Log::endl;
    Socket::debug = &_debug;

    int screenX = _args.contains("left") ?
                  _args.getInt("left") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenY = _args.contains("top") ?
                  _args.getInt("top") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenW = _args.contains("width") ?
                  _args.getInt("width") :
                  SCREEN_WIDTH;
    int screenH = _args.contains("height") ?
                  _args.getInt("height") :
                  SCREEN_HEIGHT;
    _window = SDL_CreateWindow("Client", screenX, screenY, screenW, screenH, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    screen = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(screen, 0, 0, 0, 0xff);

    _entities.insert(&_character);

    _defaultFont = TTF_OpenFont("trebuc.ttf", 16);

    OtherUser::setImage("Images/man.bmp");
    Branch::setImage("Images/branch.bmp");
    _invLabel = SDL_CreateTextureFromSurface(screen, TTF_RenderText_Solid(_defaultFont, "Inventory", Color::WHITE));

    // Randomize player name if not supplied
    if (_args.contains("username"))
        _username = _args.getString("username");
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
    if (_invLabel)
        SDL_DestroyTexture(_invLabel);
    if (screen)
        SDL_DestroyRenderer(screen);
    if (_window)
        SDL_DestroyWindow(_window);
}

void Client::checkSocket(){
    if (_invalidUsername)
        return;

    // Ensure connected to server
    if (!_connected && _timeSinceConnectAttempt >= CONNECT_RETRY_DELAY) {
        _timeSinceConnectAttempt = 0;
        // Server details
        sockaddr_in serverAddr;
        serverAddr.sin_addr.s_addr = _args.contains("server-ip") ?
                                     inet_addr(_args.getString("server-ip").c_str()) :
                                     inet_addr("127.0.0.1");
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = _args.contains("server-port") ?
                              _args.getInt("server-port") :
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

    if (!_window)
        return;

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

            case SDL_MOUSEMOTION:
                _mouse.x = e.motion.x;
                _mouse.y = e.motion.y;
                break;

            case SDL_MOUSEBUTTONUP:
                if (_loaded) {
                    // Check whether clicking a branch
                    for (std::set<Branch>::const_iterator it = _branches.begin(); it != _branches.end(); ++it) {
                        if (collision(_mouse, makeRect(it->location().x, it->location().y, 20, 20))) {
                            sendMessage(CL_COLLECT_BRANCH, makeArgs(it->serial()));
                            break;
                        }
                    }
                }
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
                double
                    dist = delta * MOVEMENT_SPEED,
                    diagDist = dist / SQRT_2;
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
                setEntityLocation(_character, newLoc);
                _locationChanged = true;
            }
        }

        // Update locations of other users
        for (std::map<std::string, OtherUser>::iterator it = _otherUsers.begin(); it != _otherUsers.end(); ++it)
            it->second.setLocation(_entities, it->second.interpolatedLocation(delta));

        checkSocket();
        // Draw
        draw();
        SDL_Delay(10);
    }
}

void Client::draw(){
    if (!_connected || !_loaded){
        SDL_RenderClear(screen);
        _debug.draw(screen);
        SDL_RenderPresent(screen);
        return;
    }

    // Background
    static const Color backgroundColor = Color::GREEN/4;
    SDL_SetRenderDrawColor(screen, backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), 0xff);
    SDL_RenderFillRect(screen, 0);

    // Entities, sorted from back to front
    for (Entity::set_t::const_iterator it = _entities.begin(); it != _entities.end(); ++it)
        (*it)->draw();

    // Rectangle around user
    SDL_SetRenderDrawColor(screen, 0xff, 0xff, 0xff, 0xff); // White
    SDL_Rect drawLoc = _character.drawRect();
    SDL_RenderDrawRect(screen, &drawLoc);

    // Other users' names
    for (std::map<std::string, OtherUser>::iterator it = _otherUsers.begin(); it != _otherUsers.end(); ++it){
        const Entity &entity = it->second.entity();
        SDL_Surface *nameSurface = TTF_RenderText_Solid(_defaultFont, it->first.c_str(), Color::CYAN);
        SDL_Texture *nameTexture = SDL_CreateTextureFromSurface(screen, nameSurface);
        SDL_Rect drawLoc = entity.location();
        drawLoc.y -= 60;
        drawLoc.x -= nameSurface->w/2;
        SDL_RenderCopy(screen, nameTexture, 0, &drawLoc);
        SDL_DestroyTexture(nameTexture);
        SDL_FreeSurface(nameSurface);
    }

    // Inventory
    int screenW, screenH;
    SDL_GetRendererOutputSize(screen, &screenW, &screenH);
    static SDL_Rect invBackgroundRect = makeRect(screenW - 250, screenH - 70, 250, 60);
    SDL_SetRenderDrawColor(screen, 0x3f, 0x3f, 0x3f, 0xff); // White/4
    SDL_RenderFillRect(screen, &invBackgroundRect);
    SDL_Rect labelRect = invBackgroundRect;
    SDL_QueryTexture(_invLabel, 0, 0, &labelRect.w, &labelRect.h);
    SDL_RenderCopy(screen, _invLabel, 0, &labelRect);
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        SDL_Rect iconRect = makeRect(screenW - 248 + i*50, screenH - 48, 48, 48);
        SDL_SetRenderDrawColor(screen, 0, 0, 0, 0xff); // Black
        SDL_RenderFillRect(screen, &iconRect);
        std::set<Item>::iterator it = _items.find(_inventory[i].first);
        if (it == _items.end())
            _debug << Color::RED << "Unknown item: " << _inventory[i].first;
        else {
            SDL_RenderCopy(screen, it->icon(), 0, &iconRect);
            if (it->stackSize() > 1) {
                // Display stack size
                SDL_Surface *qtySurface = TTF_RenderText_Solid(_defaultFont,
                                                               makeArgs(_inventory[i].second).c_str(),
                                                               Color::WHITE);
                SDL_Texture *qtyTexture = SDL_CreateTextureFromSurface(screen, qtySurface);
                SDL_Rect qtyRect;
                SDL_QueryTexture(qtyTexture, 0, 0, &qtyRect.w, &qtyRect.h);
                qtyRect.x = iconRect.x + 48 - qtyRect.w;
                qtyRect.y = iconRect.y + 48 - qtyRect.h;
                SDL_RenderCopy(screen, qtyTexture, 0, &qtyRect);
                SDL_DestroyTexture(qtyTexture);
                SDL_FreeSurface(qtySurface);
            }
        }
    }

    // FPS/latency
    std::ostringstream oss;
    if (_timeElapsed > 0)
        oss << 1000/_timeElapsed;
    else
        oss << "infinite ";
    oss << "fps " << _latency << "ms";
    SDL_Surface *statsDisplaySurface = TTF_RenderText_Solid(_defaultFont, oss.str().c_str(), Color::YELLOW);
    SDL_Texture *statsDisplay = SDL_CreateTextureFromSurface(screen, statsDisplaySurface);
    int statsWidth;
    SDL_QueryTexture(statsDisplay, 0, 0, &statsWidth, 0);
    SDL_Rect statsRect = {screenW - statsWidth, 0, 0, 0};
    SDL_RenderCopy(screen, statsDisplay, 0, &statsRect);
    SDL_DestroyTexture(statsDisplay);
    SDL_FreeSurface(statsDisplaySurface);

    _debug.draw(screen);
    SDL_RenderPresent(screen);
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
            std::map<std::string, OtherUser>::iterator it = _otherUsers.find(name);
            if (it != _otherUsers.end()) {
                _entities.erase(&it->second.entity());
                _otherUsers.erase(name);
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
                setEntityLocation(_character, p);
                _loaded = true;
            } else {
                if (_otherUsers.find(name) == _otherUsers.end()) {
                    // Create new OtherUser
                    _otherUsers[name].setLocation(_entities, p);
                }
                _otherUsers[name].destination(p);
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
            std::pair<std::set<Branch>::iterator, size_t> ret =
                _branches.insert(Branch(serial, Point(x, y)));
            if (ret.second == 1) {
                // A new branch was added; add entity to list
                _entities.insert(&ret.first->entity());
            }
            break;
        }

        case SV_REMOVE_BRANCH:
        {
            int serial;
            singleMsg >> serial >> del;
            if (del != ']')
                break;
            std::set<Branch>::const_iterator it = _branches.find(serial);
            if (it == _branches.end()){
                _debug << Color::YELLOW << "Server removed a branch we didn't know about." << Log::endl;
                assert(false);
                break; // We didn't know about this branch
            }
            _entities.erase(&it->entity());
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

void Client::setEntityLocation(Entity &entity, const Point &newLocation){
    entity.setLocation(_entities, newLocation);
}
