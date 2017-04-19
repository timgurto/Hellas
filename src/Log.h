#ifndef LOG_H
#define LOG_H

#include <fstream>
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
    Log(const std::string &logFileName = "");
    virtual ~Log(){}

    virtual void operator()(const std::string &message, const Color &color = Color::NO_KEY) = 0;

    enum LogSpecial {
        endl,
        uncolor
    };

    template<typename T>
    Log &operator<<(const T &val){
        std::ostringstream oss;
        oss << val;
        *this << oss.str();
        return *this;
    }
    virtual Log &operator<<(const std::string &val) = 0;

    /*
    Handle special commands:
      endl: end message and begin a new one
      uncolor: revert to the default message color
    */
    virtual Log &operator<<(const LogSpecial &val) = 0;
    // color: set color of current compilation
    virtual Log &operator<<(const Color &c) = 0;

protected:
    template<typename T>
    void writeToFile(const T &val) const {
        if (_logFileName.empty())
            return;
        std::ofstream of(_logFileName, std::ios_base::app);
        of << val;
        of.close();
    }
    
    template<typename T>
    void writeLineToFile(const T &val) const {
        if (_logFileName.empty())
            return;
        std::ofstream of(_logFileName, std::ios_base::app);
        of << val << std::endl;
        of.close();
    }

private:
    std::string _logFileName;
};

#endif
