#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "messageCodes.h"

struct Message{
    MessageCode code;
    std::string args;

    Message(MessageCode codeArg = NO_CODE, const std::string argsArg = ""):
        code(codeArg), args(argsArg){}
};

#endif
