#include "LogSDL.h"
#include "Client.h"
#include "ui/Label.h"
#include "ui/List.h"

LogSDL::LogSDL(const std::string &logFileName) : Log(logFileName) {}

void LogSDL::operator()(const std::string &message, const Color &color) {
  writeLineToFile(message);
  Client::_instance->addChatMessage(message, color);
}

LogSDL &LogSDL::operator<<(const std::string &val) {
  _oss << val;
  return *this;
}

LogSDL &LogSDL::operator<<(const LogSpecial &val) {
  switch (val) {
    case endl:
      operator()(_oss.str(), _compilationColor);
      _oss.str("");
      _compilationColor =
          Color::CHAT_DEFAULT;  // reset color for next compilation
      break;

    case uncolor:
      _compilationColor = Color::CHAT_DEFAULT;
      break;
  }
  return *this;
}

LogSDL &LogSDL::operator<<(const Color &c) {
  _compilationColor = c;
  return *this;
}
