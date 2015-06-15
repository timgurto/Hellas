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
    This can be done at any time.

log << endl;
    End the current compilation, adding it to the screen.
*/

class Log{
public:
    Log();
    Log(unsigned displayLines, const std::string &fontName = "trebuc.ttf", int fontSize = 16, const Color &color = Color::WHITE);
    ~Log();
    void operator()(const std::string &message, const Color *color = 0);

    enum LogEndType{endl};

    template<typename T>
    Log &operator<<(const T &val) {
        _oss << val;
        return *this;
    }
    template<> // endl: end message and begin a new one
    Log &operator<<(const LogEndType &val) {
        operator()(_oss.str(), &_compilationColor);
        _oss.str("");
        _compilationColor = _color; // reset color for next compilation
        return *this;
    }
    template<> // color: set color of current compilation
    Log &operator<<(const Color &c) {
        _compilationColor = c;
        return *this;
    }

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
