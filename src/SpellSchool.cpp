#include <cassert>
#include <map>

#include "Color.h"
#include "SpellSchool.h"

SpellSchool::SpellSchool(const std::string &name) {
    static const auto map = std::map<std::string, Type>{
        { ""s, PHYSICAL },
        { "physical"s, PHYSICAL },
        { "air"s, AIR },
        { "earth"s, EARTH },
        { "fire"s, FIRE },
        { "water"s, WATER }
    };
    auto it = map.find(name);
    if (it != map.end())
        _type = it->second;
}

SpellSchool::operator std::string() const {
    switch (_type) {
    case AIR:
        return "Air";
    case EARTH:
        return "Earth";
    case FIRE:
        return "Fire";
    case WATER:
        return "Water";
    }
    assert(false);
    return{};
}

const Color & SpellSchool::color() const {
    switch (_type) {
    case AIR:
        return Color::AIR;
    case EARTH:
        return Color::EARTH;
    case FIRE:
        return Color::FIRE;
    case WATER:
        return Color::WATER;
    }
    assert(false);
    return{Color::OUTLINE};
}

std::istream &operator >> (std::istream &lhs, SpellSchool &rhs) {
    auto name = ""s;
    lhs >> name;
    rhs = { name };
    return lhs;
}

std::string operator+ (const std::string &lhs, const SpellSchool &rhs) {
    return lhs + std::string{ rhs };
}
