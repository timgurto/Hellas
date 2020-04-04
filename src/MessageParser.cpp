#include "MessageParser.h"

#include "messageCodes.h"

bool MessageParser::hasAnotherMessage() { return iss.peek() == MSG_START; }
