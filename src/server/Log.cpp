// (C) 2015 Tim Gurto

#include "Log.h"
#include "Server.h"

Color Log::defaultColor = Color::NO_KEY;

void Log::operator()(const std::string &message, const Color &color){
    std::cout << message << std::endl;
    if (_logFile.is_open())
        _logFile << message << std::endl;

    const Color &msgColor = (&color == &defaultColor) ? _color : color;
    const Texture msgTexture(_font, message, msgColor);
    if (!msgTexture)
        return;

    _messages.push_back(msgTexture);
    if (_messages.size() > _maxMessages){
        _messages.pop_front();
    }
}

Log &Log::operator<<(const Color &c) {
    return *this;
}

Log &Log::operator<<(const LogEndType &val) {
    std::cout << val;
    return *this;
}
    
