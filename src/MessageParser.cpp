#include "MessageParser.h"

bool MessageParser::hasAnotherMessage() { return iss.peek() == MSG_START; }

MessageCode MessageParser::nextMessage() {
  auto code = int{};
  iss >> _delimiter >> code >> _delimiter;
  return static_cast<MessageCode>(code);
}
