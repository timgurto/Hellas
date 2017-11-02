#pragma once

#include <iostream>
#include <string>

class Color;

using namespace std::string_literals;

class SpellSchool {
public:
    enum Type {
        PHYSICAL,
        AIR,
        EARTH,
        FIRE,
        WATER
    };

    SpellSchool() : _type(PHYSICAL) {}
    SpellSchool(Type type) : _type(type) {}
    SpellSchool(const std::string &name);
    operator std::string() const;
    bool operator==(SpellSchool rhs) const { return _type == rhs._type; }
    bool isMagic() const { return _type != PHYSICAL; }
    const Color &color() const;

private:
    Type _type = PHYSICAL;

    friend std::istream &operator >> (std::istream &lhs, SpellSchool &rhs);
    friend std::string operator+ (const std::string&lhs, const SpellSchool &rhs);
};

std::istream &operator >> (std::istream &lhs, SpellSchool &rhs);

std::string operator+ (const std::string&lhs, const SpellSchool &rhs);
