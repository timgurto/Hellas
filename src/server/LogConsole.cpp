// (C) 2015 Tim Gurto

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

void LogConsole::operator()(const std::string &message, const Color &color){
    std::cout << message << std::endl;
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
    return *this;
}

LogConsole &LogConsole::operator<<(const LogEndType &val) {
    std::cout << std::endl;
    if (_logFile.is_open())
        _logFile << std::endl;
    return *this;
}
    
