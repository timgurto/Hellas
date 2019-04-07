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
    default:
      return "Physical";
  }
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
    default:
      return "physical";
  }
}

const Color &SpellSchool::color() const {
  switch (_type) {
    case AIR:
      return Color::STAT_AIR;
    case EARTH:
      return Color::STAT_EARTH;
    case FIRE:
      return Color::STAT_FIRE;
    case WATER:
      return Color::STAT_WATER;
  }
  return {Color::CHAT_ERROR};
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
