// (C) 2015 Tim Gurto

#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <sstream>
#include <string>

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

private:
    std::ostringstream _oss;
    Color _color;
    Color _compilationColor; // for use while compiling a message
};

#endif
