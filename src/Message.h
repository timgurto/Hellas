#ifndef MESSAGE_H
#define MESSAGE_H

#include <sstream>
#include <string>

#include "messageCodes.h"

struct Message {
  MessageCode code;
  std::string args;

  template <typename T>
  Message(MessageCode codeArg, const T& singleArg)
      : code(codeArg), args(toString(singleArg)) {}
  Message(MessageCode codeArg = NO_CODE, const std::string argsArg = {})
      : code(codeArg), args(argsArg) {}

  std::string compile() const {
    std::ostringstream oss;
    oss << MSG_START << code;
    if (args != "") oss << MSG_DELIM << args;
    oss << MSG_END;

    return oss.str();
  }
};

std::ostream& operator<<(std::ostream& lhs, const Message& rhs);

#endif
