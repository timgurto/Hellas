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

  template <typename T1>
  bool parseArgs(T1 &arg1);

  template <typename T1, typename T2>
  bool parseArgs(T1 &arg1, T2 &arg2);

  template <typename T1, typename T2, typename T3>
  bool parseArgs(T1 &arg1, T2 &arg2, T3 &arg3);

  std::istringstream iss;  // TODO: make private

 private:
  char _delimiter{0};

  template <typename T>
  void parseSingleArg(T &arg, bool isLastArg = false);

  template <>
  void parseSingleArg(std::string &arg, bool isLastArg);
};

#include "MessageParser.inl"
