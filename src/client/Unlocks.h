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
  void linkToKnownRecipes(const KnownEffects &knownRecipes);
  void linkToKnownConstructions(const KnownEffects &knownConstructions);

  enum TriggerType { CRAFT, ACQUIRE, CONSTRUCT, GATHER };
  enum EffectType { RECIPE, CONSTRUCTION };
  enum UnlockChance { HIGH_CHANCE, MODERATE_CHANCE, SMALL_CHANCE };

  struct Trigger {
    TriggerType type;
    std::string id;
  };

  struct Effect {
    EffectType type;
    std::string id;
  };

  void add(const Trigger &trigger, const Effect &effect, double chance = 1.0);

  struct EffectInfo {
    bool hasEffect{false};
    Color color;
    std::string message;
    double rawChance{0.0};
    UnlockChance chance;
  };

  static std::string chanceName(UnlockChance chance);
  static Color chanceColor(UnlockChance chance);

  EffectInfo getEffectInfo(const Trigger &trigger) const;

  using Container = std::map<Trigger, std::map<Effect, double> >;

 private:
  Container _container;
  const KnownEffects *_knownRecipes, *_knownConstructions;
};

bool operator<(const Unlocks::Trigger &lhs, const Unlocks::Trigger &rhs);
bool operator<(const Unlocks::Effect &lhs, const Unlocks::Effect &rhs);
