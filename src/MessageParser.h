#pragma once

#include <sstream>

#include "messageCodes.h"

// This class manages an input stream containing network Messages, and provides
// functions to read them.  The number and types of arguments are flexible.
class MessageParser {
 public:
  MessageParser(const std::string &input) : iss(input) {}

  bool hasAnotherMessage();
  MessageCode nextMessage();
  char getLastDelimiterRead() const { return _delimiter; }  // TODO: remove

  template <typename T>
  void parseSingleArg(T &arg, bool isLastArg = false) {
    iss >> arg;
  }

  template <>
  void parseSingleArg(std::string &arg, bool isLastArg) {
    static const size_t BUFFER_SIZE = 1023;
    char buffer[BUFFER_SIZE + 1];
    const auto expectedDelimiter = isLastArg ? MSG_END : MSG_DELIM;
    iss.get(buffer, BUFFER_SIZE, expectedDelimiter);
    arg = {buffer};
  }

  template <typename T1>
  bool parseArgs(T1 &arg1) {
    char del;

    parseSingleArg(arg1, true);
    iss >> del;
    if (del != MSG_END) return false;

    return true;
  }

  template <typename T1, typename T2>
  bool parseArgs(T1 &arg1, T2 &arg2) {
    char del;
    parseSingleArg(arg1);
    iss >> del;
    if (del != MSG_DELIM) return false;

    parseSingleArg(arg2, true);
    iss >> del;
    if (del != MSG_END) return false;

    return true;
  }

  template <typename T1, typename T2, typename T3>
  bool parseArgs(T1 &arg1, T2 &arg2, T3 &arg3) {
    char del;
    parseSingleArg(arg1);
    iss >> del;
    if (del != MSG_DELIM) return false;

    parseSingleArg(arg3);
    iss >> del;
    if (del != MSG_DELIM) return false;

    parseSingleArg(arg2, true);
    iss >> del;
    if (del != MSG_END) return false;

    return true;
  }

  std::istringstream iss;  // TODO: make private

 private:
  char _delimiter{0};
};
