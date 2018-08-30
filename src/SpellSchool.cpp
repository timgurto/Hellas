#include <cassert>
#include <map>

#include "Color.h"
#include "SpellSchool.h"

SpellSchool::SpellSchool(const std::string &name) {
  static const auto map = std::map<std::string, Type>{
      {""s, PHYSICAL},   {"physical"s, PHYSICAL}, {"air"s, AIR},
      {"earth"s, EARTH}, {"fire"s, FIRE},         {"water"s, WATER}};
  auto it = map.find(name);
  if (it != map.end()) _type = it->second;
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
  return {};
}

std::string SpellSchool::midSentenceString() const {
  switch (_type) {
    case AIR:
      return "air";
    case EARTH:
      return "earth";
    case FIRE:
      return "fire";
    case WATER:
      return "water";
    case PHYSICAL:
      return "physical";
  }
  assert(false);
  return {};
}

const Color &SpellSchool::color() const {
  switch (_type) {
    case AIR:
      return Color::TODO;
    case EARTH:
      return Color::TODO;
    case FIRE:
      return Color::TODO;
    case WATER:
      return Color::TODO;
  }
  assert(false);
  return {Color::TODO};
}

std::istream &operator>>(std::istream &lhs, SpellSchool &rhs) {
  auto name = ""s;
  lhs >> name;
  rhs = {name};
  return lhs;
}

std::string operator+(const std::string &lhs, const SpellSchool &rhs) {
  return lhs + std::string{rhs};
}
