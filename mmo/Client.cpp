#include <SDL.h>
#include <string>
#include <sstream>

#include "Client.h"
#include "messageCodes.h"
#include "util.h"

const int Client::BUFFER_SIZE = 100;
const int Client::SCREEN_WIDTH = 640;
const int Client::SCREEN_HEIGHT = 480;

const Uint32 Client::MAX_TICK_LENGTH = 100;
const double Client::MOVEMENT_SPEED = 80;
const Uint32 Client::TIME_BETWEEN_LOCATION_UPDATES = 250;

Client::Client(const Args &args):
_args(args),
_location(0, 0),
_loop(true),
_debug(SCREEN_HEIGHT/20),
_socket(&_debug),
_connected(false),
_invalidUsername(false),
_timeSinceLocUpdate(0),
_locationChanged(false){
    _debug << args << Log::endl;

    int screenX = _args.contains("left") ?
                  _args.getInt("left") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenY = _args.contains("top") ?
                  _args.getInt("top") :
                  SDL_WINDOWPOS_UNDEFINED;
    _window = SDL_CreateWindow("Client", screenX, screenY, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    _screen = SDL_GetWindowSurface(_window);

    _image = SDL_LoadBMP("Images/man.bmp");

    // Randomize player name if not supplied
    if (_args.contains("username"))
        _username = _args.getString("username");
    else
        for (int i = 0; i != 3; ++i)
            _username.push_back('a' + rand() % 26);
    _debug << "Player name: " << _username << Log::endl;

    _debug("Client initialized");

    _defaultFont = TTF_OpenFont("trebuc.ttf", 16);
}

Client::~Client(){
    if (_defaultFont)
        TTF_CloseFont(_defaultFont);
    if (_image)
        SDL_FreeSurface(_image);
    if (_window)
        SDL_DestroyWindow(_window);
}

void Client::checkSocket(){
    if (_invalidUsername)
        return;

    // Ensure connected to server
    if (!_connected) {
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
            _connected = true;
            // Announce player name
            std::ostringstream oss;
            oss << '[' << CL_I_AM << ',' << _username << ']';
            _socket.sendMessage(oss.str());
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
        int charsRead = recv(_socket.getRaw(), buffer, 100, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0){
            buffer[charsRead] = '\0';
            _debug << "recv: " << std::string(buffer) << "" << Log::endl;
            _messages.push(std::string(buffer));
        }
    }
}

void Client::run(){

    if (!_window || !_image)
        return;

    Uint32 timeAtLastTick = SDL_GetTicks();
    while (_loop) {
        Uint32 newTime = SDL_GetTicks();
        Uint32 timeElapsed = newTime - timeAtLastTick;
        if (timeElapsed > MAX_TICK_LENGTH)
            timeElapsed = MAX_TICK_LENGTH;
        double delta = timeElapsed  / 1000.0;
        timeAtLastTick = newTime;

        _timeSinceLocUpdate += timeElapsed;
        if (_locationChanged && _timeSinceLocUpdate > TIME_BETWEEN_LOCATION_UPDATES) {
            std::ostringstream oss;
            oss << '[' << CL_LOCATION << ',' << _location.x << ',' << _location.y << ']';
            _socket.sendMessage(oss.str());
            _locationChanged = false;
            _timeSinceLocUpdate = 0;
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

            default:
                // Unhandled event
                ;
            }
        }
        // Poll keys (whether they are currently pressed, not key events)
        const Uint8 *keyboardState = SDL_GetKeyboardState(0);
        bool
            up = keyboardState[SDL_SCANCODE_UP] == SDL_PRESSED,
            down = keyboardState[SDL_SCANCODE_DOWN] == SDL_PRESSED,
            left = keyboardState[SDL_SCANCODE_LEFT] == SDL_PRESSED,
            right = keyboardState[SDL_SCANCODE_RIGHT] == SDL_PRESSED;
            if (up != down || left != right) {
            double
                dist = delta * MOVEMENT_SPEED,
                diagDist = dist / SQRT_2;
            if (up && !down)
                _location.y -= (left != right) ? diagDist : dist;
            else if (down && !up)
                _location.y += (left != right) ? diagDist : dist;
            if (left && !right)
                _location.x -= (up != down) ? diagDist : dist;
            else if (right && !left)
                _location.x += (up != down) ? diagDist : dist;
            _locationChanged = true;
        }

        checkSocket();
        // Draw
        draw();
        if (!_connected)
            SDL_Delay(1000);
        SDL_Delay(10);
    }
}

void Client::draw(){
    if (!_connected){
        SDL_FillRect(_screen, 0, Color::BLACK);
        _debug.draw(_screen);
        SDL_UpdateWindowSurface(_window);
        return;
    }
    SDL_FillRect(_screen, 0, Color::GREEN/4);
    SDL_Rect drawLoc = _location;
    SDL_Rect borderRect = drawLoc;
    borderRect.x = borderRect.x - 1;
    borderRect.y = borderRect.y - 1;
    borderRect.w = 22;
    borderRect.h = 42;
    SDL_FillRect(_screen, &borderRect, Color::WHITE);
    SDL_BlitSurface(_image, 0, _screen, &drawLoc);
    for (std::map<std::string, Point>::iterator it = _otherUserLocations.begin(); it != _otherUserLocations.end(); ++it){
        drawLoc = it->second;
        SDL_BlitSurface(_image, 0, _screen, &drawLoc);
        SDL_Color color = {0, 0xff, 0xff};
        SDL_Surface *nameSurface = TTF_RenderText_Solid(_defaultFont, it->first.c_str(), color);
        drawLoc.y -= 20;
        SDL_BlitSurface(nameSurface, 0, _screen, &drawLoc);
        SDL_FreeSurface(nameSurface);
    }
    _debug.draw(_screen);
    SDL_UpdateWindowSurface(_window);
}

void Client::handleMessage(const std::string &msg){
    std::istringstream iss(msg);
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    while (iss.peek() == '[') {
        iss >>del >> msgCode >> del;
        switch(msgCode) {

        case SV_LOCATION:
        {
            std::string name;
            double x, y;
            iss.get(buffer, BUFFER_SIZE, ',');
            name = std::string(buffer);
            iss >> del >> x >> del >> y >> del;
            if (del != ']')
                return;
            if (name == _username)
                _location = Point(x, y);
            else
                _otherUserLocations[name] = Point(x, y);
            break;
        }

        case SV_USER_DISCONNECTED:
        {
            std::string name;
            iss.get(buffer, BUFFER_SIZE, ']');
            name = std::string(buffer);
            iss >> del;
            if (del != ']')
                return;
            _otherUserLocations.erase(name);
            _debug << name << " disconnected." << Log::endl;
            break;
        }

        case SV_DUPLICATE_USERNAME:
            if (del != ']')
                return;
            _invalidUsername = true;
            _debug << Color::RED << "The user " << _username << " is already connected to the server." << Log::endl;
            break;

        case SV_INVALID_USERNAME:
            if (del != ']')
                return;
            _invalidUsername = true;
            _debug << Color::RED << "The username " << _username << " is invalid." << Log::endl;
            break;

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }
    }
}
