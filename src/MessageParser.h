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

  std::istringstream iss;  // TODO: make private

 private:
  char _delimiter{0};
};
