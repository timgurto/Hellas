// (C) 2015 Tim Gurto

#include "Client.h"
#include "LogSDL.h"

const int LogSDL::DEFAULT_LINE_HEIGHT = 10;

LogSDL::LogSDL():
_valid(false){}

LogSDL::LogSDL(unsigned displayLines, const std::string &logFileName, const std::string &fontName,
               int fontSize, const Color &color):
_maxMessages(displayLines),
_font(TTF_OpenFont(fontName.c_str(), fontSize)),
_valid(true),
_color(color),
_compilationColor(color),
_lineHeight(DEFAULT_LINE_HEIGHT){
    if (!_font){
        _valid = false;
    }
    if (!logFileName.empty()) {
        _logFile.open(logFileName);
    }
}

LogSDL::~LogSDL(){
    while (!_messages.empty())
        _messages.pop_front();

    if (_font)
        TTF_CloseFont(_font);

    if (_logFile.is_open())
        _logFile.close();
}

void LogSDL::operator()(const std::string &message, const Color &color){
    if (_logFile.is_open())
        _logFile << message << std::endl;

    const Color &msgColor = (&color == &Color::NO_KEY) ? _color : color;
    const Texture msgTexture(_font, message, msgColor);
    if (!msgTexture)
        return;

    _messages.push_back(msgTexture);
    if (_messages.size() > _maxMessages){
        _messages.pop_front();
    }
}

void LogSDL::draw(int x, int y) const{
    if (!_valid)
        return;
    
    for (const Texture &message : _messages) {
        message.draw(x, y);
        y += (_lineHeight == DEFAULT_LINE_HEIGHT) ? message.height() : _lineHeight;
    }
}

LogSDL &LogSDL::operator<<(const std::string &val) {
    _oss << val;
    return *this;
}

LogSDL &LogSDL::operator<<(const LogEndType &val) {
    operator()(_oss.str(), _compilationColor);
    _oss.str("");
    _compilationColor = _color; // reset color for next compilation
    return *this;
}
    
LogSDL &LogSDL::operator<<(const Color &c) {
    _compilationColor = c;
    return *this;
}

void LogSDL::setFont(const std::string &fontName, int fontSize){
    if (_font)
        TTF_CloseFont(_font);
    _font = TTF_OpenFont(fontName.c_str(), fontSize);
    if (!_font)
        _valid = (_font != 0);
}

void LogSDL::setLineHeight(int height){
    _lineHeight = height;
}
