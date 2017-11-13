#pragma once

#include "Texture.h"

class ClientBuff {
public:
    using ID = std::string;
    using Name = std::string;

    ClientBuff() {}
    ClientBuff(const ID &id);
    void name(const Name &name) { _name = name; }

private:
    Texture _icon;
    std::string _name;
};

using ClientBuffs = std::map<ClientBuff::ID, ClientBuff>;
