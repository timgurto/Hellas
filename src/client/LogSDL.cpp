// (C) 2015 Tim Gurto

#include "Client.h"
#include "LogSDL.h"
#include "ui/Label.h"
#include "ui/List.h"

LogSDL::LogSDL(const std::string &logFileName):
_compilationColor(Color::MMO_L_GREY){
    if (!logFileName.empty()) {
        _logFile.open(logFileName);
    }
}

LogSDL::~LogSDL(){
    if (_logFile.is_open())
        _logFile.close();
}

void LogSDL::operator()(const std::string &message, const Color &color){
    if (_logFile.is_open())
        _logFile << message << std::endl;

    Client::_instance->addChatMessage(message, color);
}

LogSDL &LogSDL::operator<<(const std::string &val) {
    _oss << val;
    return *this;
}

LogSDL &LogSDL::operator<<(const LogEndType &val) {
    operator()(_oss.str(), _compilationColor);
    _oss.str("");
    _compilationColor = Color::MMO_L_GREY; // reset color for next compilation
    return *this;
}
    
LogSDL &LogSDL::operator<<(const Color &c) {
    _compilationColor = c;
    return *this;
}
