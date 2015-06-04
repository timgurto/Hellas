#include <SDL.h>
#include <string>
#include <sstream>

#include "Client.h"
#include "messageCodes.h"
#include "util.h"

const int Client::BUFFER_SIZE = 1023;
const int Client::SCREEN_WIDTH = 640;
const int Client::SCREEN_HEIGHT = 480;

const Uint32 Client::MAX_TICK_LENGTH = 100;
const Uint32 Client::SERVER_TIMEOUT = 10000;
const Uint32 Client::CONNECT_RETRY_DELAY = 1000;

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
_locationChanged(false),
_time(SDL_GetTicks()),
_timeElapsed(0),
_lastPing(_time),
_timeSinceConnectAttempt(CONNECT_RETRY_DELAY),
_loaded(false){
    _debug << args << Log::endl;

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
    _screen = SDL_GetWindowSurface(_window);

    _image = SDL_LoadBMP("Images/man.bmp");

    // Randomize player name if not supplied
    if (_args.contains("username"))
        _username = _args.getString("username");
    else
        for (int i = 0; i != 3; ++i)
            _username.push_back('a' + rand() % 26);
    _debug << "Player name: " << _username << Log::endl;

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
            _connected = true;
            _timeSinceConnectAttempt = 0;
            _lastPing = _time;
            // Announce player name
            std::ostringstream oss;
            oss << '[' << CL_I_AM << ',' << _username << ']';
            _socket.sendMessage(oss.str());
            _debug("Loading data from server");
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

    if (!_window || !_image)
        return;

    Uint32 timeAtLastTick = SDL_GetTicks();
    while (_loop) {
        _time = SDL_GetTicks();
        _timeElapsed = _time - timeAtLastTick;
        if (_timeElapsed > MAX_TICK_LENGTH)
            _timeElapsed = MAX_TICK_LENGTH;
        double delta = _timeElapsed / 1000.0;
        timeAtLastTick = _time;

        // Ensure server connectivity
        if (_connected && _time - _lastPing > SERVER_TIMEOUT) {
            _debug << Color::RED << "Disconnected from server" << Log::endl;
            _socket = Socket(&_debug);
            _connected = false;
        }

        if (!_connected) {
            _timeSinceConnectAttempt += _timeElapsed;

        } else {
            _timeSinceLocUpdate += _timeElapsed;
            if (_locationChanged && _timeSinceLocUpdate > TIME_BETWEEN_LOCATION_UPDATES) {
                std::ostringstream oss;
                oss << '[' << CL_LOCATION << ',' << _location.x << ',' << _location.y << ']';
                _socket.sendMessage(oss.str());
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

            default:
                // Unhandled event
                ;
            }
        }
        // Poll keys (whether they are currently pressed, not key events)
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
        }

        checkSocket();
        // Draw
        draw();
        SDL_Delay(10);
    }
}

void Client::draw(){
    if (!_connected || !_loaded){
        SDL_FillRect(_screen, 0, Color::BLACK);
        _debug.draw(_screen);
        SDL_UpdateWindowSurface(_window);
        return;
    }

    // Background
    SDL_FillRect(_screen, 0, Color::GREEN/4);

    // User
    SDL_Rect drawLoc = _location;
    SDL_Rect borderRect = drawLoc;
    borderRect.x = borderRect.x - 3;
    borderRect.y = borderRect.y - 3;
    borderRect.w = 26;
    borderRect.h = 46;
    SDL_FillRect(_screen, &borderRect, Color::WHITE);
    SDL_BlitSurface(_image, 0, _screen, &drawLoc);

    // Other users
    for (std::map<std::string, Point>::iterator it = _otherUserLocations.begin(); it != _otherUserLocations.end(); ++it){
        drawLoc = it->second;
        SDL_BlitSurface(_image, 0, _screen, &drawLoc);
        SDL_Color color = {0, 0xff, 0xff};
        SDL_Surface *nameSurface = TTF_RenderText_Solid(_defaultFont, it->first.c_str(), color);
        drawLoc.y -= 20;
        SDL_BlitSurface(nameSurface, 0, _screen, &drawLoc);
        SDL_FreeSurface(nameSurface);
    }

    // FPS/latency
    std::ostringstream oss;
    if (_timeElapsed > 0)
        oss << 1000/_timeElapsed;
    else
        oss << "infinite ";
    oss << "fps " << _latency << "ms";
    SDL_Surface *statsDisplay = TTF_RenderText_Solid(_defaultFont, oss.str().c_str(), Color::YELLOW);
    SDL_Rect statsRect = {SCREEN_WIDTH - statsDisplay->w, 0, 0, 0};
    SDL_BlitSurface(statsDisplay, 0, _screen, &statsRect);
    SDL_FreeSurface(statsDisplay);

    _debug.draw(_screen);
    SDL_UpdateWindowSurface(_window);
}

void Client::handleMessage(const std::string &msg){
    std::istringstream iss(msg);
    int msgCode;
    unsigned serial;
    char del;
    static char buffer[BUFFER_SIZE+1];
    // Read until a new message is found
    while (iss.peek() != '[' && !iss.eof()) // Clear out malformed stuff
        iss >> del;
    while (iss.peek() == '[') {
        iss >> del >> serial >> del >> msgCode >> del;
        switch(msgCode) {

        case SV_PING:
        {
            Uint32 t;
            iss >> t >> del;
            if (del != ']')
                break;
            _lastPing = _time;
            std::ostringstream oss;
            oss << '[' << CL_PING_REPLY << ',' << t << ',' << _time << ']';
            _socket.sendMessage(oss.str());
            break;
        }

        case SV_PING_REPLY_2:
        {
            Uint32 t;
            iss >> t >> del;
            if (del != ']')
                break;
            _latency = (_time - t) / 2;
            break;
        }

        case SV_LOCATION:
        {
            std::string name;
            double x, y;
            iss.get(buffer, BUFFER_SIZE, ',');
            name = std::string(buffer);
            iss >> del >> x >> del >> y >> del;
            if (del != ']')
                break;
            if (name == _username) {
                _location = Point(x, y);
                _loaded = true;
            } else
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
                break;
            _otherUserLocations.erase(name);
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

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }

        if (del == ']') {
        // Message successfully received; send acknowledgement
            std::ostringstream oss;
            oss << '[' << CL_ACK << ',' << serial << ']';
            _socket.sendMessage(oss.str());
        } else {
            // Malformed or cut-off message; skip until a new message is found
            while (iss.peek() != '[' && !iss.eof()){
                iss >> del;
            }
        }
    }
}
