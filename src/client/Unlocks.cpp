#include "Unlocks.h"

#include "../util.h"

using namespace std::string_literals;

bool operator<(const Unlocks::Trigger &lhs, const Unlocks::Trigger &rhs) {
  if (lhs.type != rhs.type) return lhs.type < rhs.type;
  return lhs.id < rhs.id;
}

bool operator<(const Unlocks::Effect &lhs, const Unlocks::Effect &rhs) {
  if (lhs.type != rhs.type) return lhs.type < rhs.type;
  return lhs.id < rhs.id;
}

void Unlocks::linkToKnownRecipes(const KnownEffects &knownRecipes) {
  _knownRecipes = &knownRecipes;
}

void Unlocks::linkToKnownConstructions(const KnownEffects &knownConstructions) {
  _knownConstructions = &knownConstructions;
}

void Unlocks::add(const Trigger &trigger, const Effect &effect, double chance) {
  _container[trigger][effect] = chance;
}

std::string Unlocks::chanceName(UnlockChance chance) {
  switch (chance) {
    case SMALL_CHANCE:
      return "Small";
    case MODERATE_CHANCE:
      return "Moderate";
    case HIGH_CHANCE:
      return "High";
    default:
      return ""s;
  }
}

Color Unlocks::chanceColor(UnlockChance chance) {
  switch (chance) {
    case SMALL_CHANCE:
      return Color::CHANCE_SMALL;
    case MODERATE_CHANCE:
      return Color::CHANCE_MODERATE;
    case HIGH_CHANCE:
      return Color::CHANCE_HIGH;
    default:
      return Color::WHITE;
  }
}

Unlocks::EffectInfo Unlocks::getEffectInfo(const Trigger &trigger) const {
  auto ret = EffectInfo{};

  auto it = _container.find(trigger);
  if (it == _container.end()) return ret;

  // Check against what has already been unlocked
  auto highestChance = 0.0;
  for (const auto &effectPair : it->second) {
    const auto &effect = effectPair.first;
    const auto &knownEffects =
        effect.type == RECIPE ? *_knownRecipes : *_knownConstructions;
    auto alreadyKnown = knownEffects.find(effect.id) != knownEffects.end();
    if (!alreadyKnown) {
      ret.hasEffect = true;
      highestChance = max(highestChance, effectPair.second);
    }
  }

  if (!ret.hasEffect) return ret;

  ret.rawChance = highestChance;

  auto chanceDescription = "";
  if (ret.rawChance <= 0.05)
    ret.chance = SMALL_CHANCE;
  else if (ret.rawChance <= 0.4)
    ret.chance = MODERATE_CHANCE;
  else
    ret.chance = HIGH_CHANCE;

  auto actionDescription = ""s;
  switch (trigger.type) {
    case CRAFT:
      actionDescription = "Crafting this recipe";
      break;
    case ACQUIRE:
      actionDescription = "Picking up this item";
      break;
    case GATHER:
      actionDescription = "Gathering from this object";
      break;
    case CONSTRUCT:
      actionDescription = "Constructing this object";
      break;
  }

  ret.message = actionDescription + " has a "s + chanceName(ret.chance) +
                " chance to unlock something.";
  ret.color = chanceColor(ret.chance);
  return ret;
}
