#include "ClientBuff.h"

using namespace std::string_literals;

ClientBuffType::ClientBuffType(const std::string &iconFile)
    : _icon{"Images/Icons/"s + iconFile + ".png"s} {}
