#pragma once

#include "Texture.h"

class ClientBuffType {
public:
    using ID = std::string;
    using Name = std::string;

    ClientBuffType() {}
    ClientBuffType(const ID &id);
    void name(const Name &name) { _name = name; }

private:
    Texture _icon;
    std::string _name;
};

using ClientBuffTypes = std::map<ClientBuffType::ID, ClientBuffType>;
