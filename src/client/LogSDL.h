// (C) 2015 Tim Gurto

#ifndef LOG_SDL_H
#define LOG_SDL_H

#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

#include "../Log.h"
#include "Texture.h"

// A message log that uses SDL to display colored text on the screen.
class LogSDL : public Log{
public:
    LogSDL();
    LogSDL(unsigned displayLines, const std::string &logFileName = "",
        const std::string &fontName = "trebuc.ttf", int fontSize = 16,
        const Color &color = Color::WHITE);
    ~LogSDL();
    // For default reference parameter.  Indicates that the Log's _color should be used.
    static Color defaultColor;
    void operator()(const std::string &message, const Color &color = Color::NO_KEY);

    template<typename T>
    LogSDL &operator<<(const T &val) {
        _oss << val;
        return *this;
    }
    LogSDL &operator<<(const std::string &val);
    // endl: end message and begin a new one
    LogSDL &operator<<(const LogEndType &val);
    // color: set color of current compilation
    LogSDL &operator<<(const Color &c);

    void draw(int x = 0, int y = 0) const;

    void setFont(const std::string &fontName = "trebuc.ttf", int fontSize = 16);
    void setLineHeight(int height = 16);

    static const int DEFAULT_LINE_HEIGHT;

private:
    std::list<Texture> _messages;
    unsigned _maxMessages;
    TTF_Font *_font;
    bool _valid; // false if an error has occurred
    std::ostringstream _oss;
    Color _color;
    Color _compilationColor; // for use while compiling a message
    std::ofstream _logFile;
    int _lineHeight;
};

#endif
