// (C) 2015 Tim Gurto

#include "Client.h"
#include "Server.h"
#include "Log.h"

Log::Log():
_valid(false){}

Log::Log(unsigned displayLines, const std::string &fontName, int fontSize, const Color &color):
_maxMessages(displayLines),
_font(TTF_OpenFont(fontName.c_str(), fontSize)),
_color(color),
_compilationColor(color),
_valid(true){
    if (!_font){
        std::string err = TTF_GetError();
        _valid = false;
    }
}

Log::~Log(){
    while (!_messages.empty())
        _messages.pop_front();

    if (_font)
        TTF_CloseFont(_font);
}

void Log::operator()(const std::string &message, const Color *color){
    Texture msgTexture(_font, message, color ? *color : _color);
    if (!msgTexture)
        return;

    _messages.push_back(msgTexture);
    if (_messages.size() > _maxMessages){
        _messages.pop_front();
    }
}

void Log::draw(int x, int y) const{
    if (!_valid)
        return;
    
    for (const Texture message : _messages) {
        message.draw(x, y);
        y += message.height();
    }

}
