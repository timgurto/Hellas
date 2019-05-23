#ifndef LOG_CONSOLE_H
#define LOG_CONSOLE_H

#include <fstream>
#include <iostream>
#include <string>

#include "../Log.h"

// A message log that writes to the console.
class LogConsole : public Log {
 public:
  LogConsole(const std::string &logFileName = "");

  virtual void operator()(const std::string &message,
                          const Color &color = Color::WHITE) override;

  void quiet(bool b = true) { _quiet = b; }

  template <typename T>
  LogConsole &operator<<(const T &val) {
    if (_thisLineNeedsATimestamp) writeToFile(timestamp());
    writeToFile(val);

    if (_quiet) {
      _thisLineNeedsATimestamp = false;
      return *this;
    }

    if (_thisLineNeedsATimestamp) std::cout << timestamp();
    std::cout << val << std::flush;

    _thisLineNeedsATimestamp = false;
    return *this;
  }
  LogConsole &operator<<(const std::string &val) override;
  LogConsole &operator<<(const LogSpecial &val) override;
  LogConsole &operator<<(const Color &c) override;

 private:
  bool _quiet;  // If true, suppress all messages.
  bool _thisLineNeedsATimestamp{true};
  bool _useColours{true};

  std::string colorCode(const Color &color = Color::NO_KEY);
};

#endif
