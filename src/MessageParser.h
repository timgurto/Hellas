#pragma once

#include <sstream>

// This class manages an input stream containing network Messages, and provides
// functions to read them.  The number and types of arguments are flexible.
class MessageParser {
 public:
  MessageParser(const std::string &input) : iss(input) {}

  bool hasAnotherMessage();

  std::istringstream iss;
};
