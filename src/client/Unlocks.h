#pragma once

#include <map>
#include <set>
#include <string>

#include "../Color.h"

// Describes everything that can be unlocked by a recipe, construction, etc.
// This is used to show chance-to-unlock to the user.
class Unlocks {
 public:
  using KnownEffects = std::set<std::string>;
  static void linkToKnownRecipes(const KnownEffects &knownRecipes);
  static void linkToKnownConstructions(const KnownEffects &knownConstructions);

  enum TriggerType { CRAFT, ACQUIRE, CONSTRUCT, GATHER };
  enum EffectType { RECIPE, CONSTRUCTION };

  struct Trigger {
    TriggerType type;
    std::string id;
  };

  struct Effect {
    EffectType type;
    std::string id;
  };

  static void add(const Trigger &trigger, const Effect &effect,
                  double chance = 1.0);

  struct EffectInfo {
    bool hasEffect;
    Color color;
    std::string message;
  };

  static EffectInfo getEffectInfo(const Trigger &trigger);

  using Container = std::map<Trigger, std::map<Effect, double> >;

 private:
  static Container _container;
  static const KnownEffects *_knownRecipes, *_knownConstructions;
};

bool operator<(const Unlocks::Trigger &lhs, const Unlocks::Trigger &rhs);
bool operator<(const Unlocks::Effect &lhs, const Unlocks::Effect &rhs);
