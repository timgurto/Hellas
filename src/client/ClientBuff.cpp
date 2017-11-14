#include "ClientBuff.h"

using namespace std::string_literals;

ClientBuffType::ClientBuffType(const ID & id) :
    _icon{ "Images/Spells/"s + id + ".png"s } {}
