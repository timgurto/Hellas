#include "ClientBuff.h"

using namespace std::string_literals;

ClientBuff::ClientBuff(const ID & id) :
    _icon{ "Images/Spells/"s + id + ".png"s } {}
