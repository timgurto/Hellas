#include "LogConsole.h"

#include <sstream>

#include "../Args.h"
#include "Server.h"

extern Args cmdLineArgs;

LogConsole::LogConsole(const std::string &logFileName)
    : Log(logFileName), _quiet(false) {
  if (cmdLineArgs.contains("no-colours")) _useColours = false;
}

std::string LogConsole::colorCode(const Color &color) {
  if (!_useColours) return {};

  static std::map<Color, std::string> colorCodes;
  auto ret = colorCodes.insert(std::make_pair(color, std::string()));
  if (ret.second) {  // Insert new color
    if (color == Color::NO_KEY)
      ret.first->second = "\x1B[0m";  // Return to default color
    else {
      std::ostringstream oss;
      oss << "\x1B[38;2;" << static_cast<int>(color.r()) << ';'
          << static_cast<int>(color.g()) << ';' << static_cast<int>(color.b())
          << 'm';
      ret.first->second = oss.str();
    }
  }
  return ret.first->second;
}

void LogConsole::operator()(const std::string &message, const Color &color) {
  writeLineToFile(timestamp() + message);
  if (_quiet) return;
  std::cout << colorCode(color) << timestamp() << message << colorCode()
            << std::endl;
}

LogConsole &LogConsole::operator<<(const std::string &val) {
  auto valToWrite = val;
  if (_thisLineNeedsATimestamp) valToWrite = timestamp() + valToWrite;
  _thisLineNeedsATimestamp = false;

  writeToFile(valToWrite);

  if (_quiet) return *this;

  std::cout << valToWrite << std::flush;

  return *this;
}

LogConsole &LogConsole::operator<<(const Color &c) {
  if (_quiet) return *this;
  std::cout << colorCode(c);

  return *this;
}

LogConsole &LogConsole::operator<<(const LogSpecial &val) {
  if (val == endl) {
    writeLineToFile("");
    _thisLineNeedsATimestamp = true;
  }

  if (_quiet) return *this;

  switch (val) {
    case endl:
      std::cout << colorCode() << std::endl;
      break;

    case uncolor:
      std::cout << colorCode() << std::endl;
      break;
  }
  return *this;
}
