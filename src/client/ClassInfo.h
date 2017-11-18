#pragma once

#include <map>
#include <string>

#include "Texture.h"

class ClassInfo {
public:
    using ID = std::string;
    using Name = std::string;
    using Container = std::map<ID, ClassInfo>;

    ClassInfo() {}
    ClassInfo(const ID &id, const Name &name);

    const Name &name() const { return _name; }
    const Texture &image() const { return _image; }

private:
    Name _name;
    Texture _image;
};
