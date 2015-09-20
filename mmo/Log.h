// (C) 2015 Tim Gurto

#ifndef LOG_H
#define LOG_H

#include <list>
#include <sstream>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

#include "Color.h"
#include "Texture.h"

/*
A message log which accepts, queues and displays messages to the screen
Usage:

log("message");
    Add a message to the screen.

log("message", Color::YELLOW);
    Add a colored message to the screen

log << "message " << 1;
    Compile a message of different values/types

log << Color::YELLOW;
    Change the color of the current compilation; it will reset after endl is received.
    This can be done at any time, but the one color will be used for the entire compilation.

log << endl;
    End the current compilation, adding it to the screen.
*/

class Log{
public:
    Log();
    Log(unsigned displayLines, const std::string &fontName = "trebuc.ttf", int fontSize = 16, const Color &color = Color::WHITE);
    ~Log();
    // For default reference parameter.  Indicates that the Log's _color should be used.
    static Color defaultColor;
    void operator()(const std::string &message, const Color &color = defaultColor);

    enum LogEndType{endl};

    template<typename T>
    Log &operator<<(const T &val) {
        _oss << val;
        return *this;
    }
    // endl: end message and begin a new one
    Log &operator<<(const LogEndType &val);
    // color: set color of current compilation
    Log &operator<<(const Color &c);

    void draw(int x = 0, int y = 0) const;

private:
    std::list<Texture> _messages;
    unsigned _maxMessages;
    TTF_Font *_font;
    bool _valid; // false if an error has occurred
    std::ostringstream _oss;
    Color _color;
    Color _compilationColor; // for use while compiling a message
};

#endif
