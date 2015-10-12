// (C) 2015 Tim Gurto

#include "Log.h"
#include "Server.h"

const Color Log::defaultColor = Color::NO_KEY;

Log::Log(const std::string &logFileName){
    if (!logFileName.empty())
        _logFile.open(logFileName);
}

Log::~Log(){
    if (_logFile.is_open())
        _logFile.close();
}

void Log::operator()(const std::string &message, const Color &color){
    std::cout << message << std::endl;
    if (_logFile.is_open())
        _logFile << message << std::endl;
}

Log &Log::operator<<(const Color &c) {
    return *this;
}

Log &Log::operator<<(const LogEndType &val) {
    std::cout << val;
    if (_logFile.is_open())
        _logFile << std::endl;
    return *this;
}
    
