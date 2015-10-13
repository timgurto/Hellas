// (C) 2015 Tim Gurto

#ifndef LOG_H
#define LOG_H

#include <sstream>
#include <string>

#include "Color.h"

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
    virtual ~Log(){}

    void operator()(const std::string &message, const Color &color = Color::NO_KEY);

    enum LogEndType{endl};

    template<typename T>
    Log &operator<<(const T &val){
        std::ostringstream oss;
        oss << val;
        *this << oss.str();
        return *this;
    }
    virtual Log &operator<<(const std::string &val) = 0;
    // endl: end message and begin a new one
    virtual Log &operator<<(const LogEndType &val) = 0;
    // color: set color of current compilation
    virtual Log &operator<<(const Color &c) = 0;
};

#endif
