// (C) 2015 Tim Gurto

#include <sstream>

#include "LogConsole.h"
#include "Server.h"

LogConsole::LogConsole(const std::string &logFileName){
    if (!logFileName.empty())
        _logFile.open(logFileName);
}

LogConsole::~LogConsole(){
    if (_logFile.is_open())
        _logFile.close();
}

static const std::string &colorCode(const Color &color = Color::NO_KEY){
    static std::map<Color, std::string> colorCodes;
    auto ret = colorCodes.insert(std::make_pair(color, std::string()));
    if (ret.second) { // Insert new color
        if (color == Color::NO_KEY)
            ret.first->second = "\x1B[0m"; // Return to default color
        else {
            std::ostringstream oss;
            oss << "\x1B[38;2;"
                << static_cast<int>(color.r()) << ';'
                << static_cast<int>(color.g()) << ';'
                << static_cast<int>(color.b()) << 'm';
            ret.first->second = oss.str();
        }
    }
    return ret.first->second;
}

void LogConsole::operator()(const std::string &message, const Color &color){
    std::cout << colorCode(color) << message << colorCode() << std::endl;
    if (_logFile.is_open())
        _logFile << message << std::endl;
}

LogConsole &LogConsole::operator<<(const std::string &val) {
    std::cout << val;
    if (_logFile.is_open())
        _logFile << val;
    return *this;
}

LogConsole &LogConsole::operator<<(const Color &c) {
    std::cout << colorCode(c);
    return *this;
}

LogConsole &LogConsole::operator<<(const LogEndType &val) {
    std::cout << colorCode() << std::endl;
    if (_logFile.is_open())
        _logFile << std::endl;
    return *this;
}
    
