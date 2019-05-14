#include "Unlocks.h"

#include "../util.h"

using namespace std::string_literals;

Unlocks::Container Unlocks::_container;
const Unlocks::KnownEffects *Unlocks::_knownRecipes{nullptr};
const Unlocks::KnownEffects *Unlocks::_knownConstructions{nullptr};

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

Unlocks::EffectInfo Unlocks::getEffectInfo(const Trigger &trigger) {
  auto ret = EffectInfo{false, {}, {}};

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
      break;
    }
  }

  if (!ret.hasEffect) return ret;

  auto chanceDescription = "";
  if (highestChance <= 0.05) {
    chanceDescription = "small";
  } else if (highestChance <= 0.4) {
    chanceDescription = "moderate";
  } else {
    chanceDescription = "high";
  }
  ret.color = Color::TOOLTIP_BODY;

  auto actionDescription = ""s;
  switch (trigger.type) {
    case CRAFT:
      actionDescription = "Crafting this recipe";
      break;
    case ACQUIRE:
      actionDescription = "Picking up this item";
      break;
    case GATHER:
      actionDescription = "Gathering this item";
      break;
    case CONSTRUCT:
      actionDescription = "Constructing this object";
      break;
  }

  ret.message = actionDescription + " has a "s + chanceDescription +
                " chance to unlock something.";
  return ret;
}
