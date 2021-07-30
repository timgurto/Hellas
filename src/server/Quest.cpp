#include "Quest.h"

#include "Server.h"
#include "User.h"

Quest::Objective::Type Quest::Objective::typeFromString(
    const std::string& asString) {
  const auto typesByName =
      std::map<std::string, Quest::Objective::Type>{{"kill", KILL},
                                                    {"fetch", FETCH},
                                                    {"construct", CONSTRUCT},
                                                    {"cast", CAST_SPELL}};
  auto it = typesByName.find(asString);
  if (it == typesByName.end()) return NONE;
  return it->second;
}

void Quest::Objective::setType(const std::string& asString) {
  type = typeFromString(asString);
}

std::string Quest::Objective::typeAsString() const {
  const auto typeNames =
      std::map<Quest::Objective::Type, std::string>{{KILL, "kill"},
                                                    {FETCH, "fetch"},
                                                    {CONSTRUCT, "construct"},
                                                    {CAST_SPELL, "cast"}};
  auto it = typeNames.find(type);
  if (it == typeNames.end()) return "none";
  return it->second;
}

bool Quest::canBeCompletedByUser(const User& user) const {
  for (const auto& objective : objectives) {
    switch (objective.type) {
      case Objective::NONE:
        SERVER_ERROR("Checking objective with no type");
        return false;

      case Objective::FETCH: {
        auto requiredItem = ItemSet{};
        requiredItem.add(objective.item, objective.qty);
        if (!user.hasItems(requiredItem)) return false;
        continue;
      }

      case Objective::KILL:
      case Objective::CONSTRUCT:
      case Objective::CAST_SPELL:
        if (user.questProgress(id, objective.type, objective.id) <
            objective.qty)
          return false;
        continue;
    }
  }
  return true;
}

XP Quest::getXPFor(Level userLevel) const {
  const auto difficulty = level - userLevel;

  if (difficulty <= -10) return 0;
  if (difficulty >= 10) return 500;
  auto xpToReward = 250 + difficulty * 25;

  if (elite) xpToReward *= 2;

  return xpToReward;
}
