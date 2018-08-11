#include "Quest.h"
#include "Server.h"
#include "User.h"

Quest::Objective::Type Quest::Objective::typeFromString(
    const std::string& asString) {
  const auto typesByName = std::map<std::string, Quest::Objective::Type>{
      {"kill", KILL}, {"fetch", FETCH}, {"construct", CONSTRUCT}};
  auto it = typesByName.find(asString);
  if (it == typesByName.end()) return NONE;
  return it->second;
}

void Quest::Objective::setType(const std::string& asString) {
  type = typeFromString(asString);
}

std::string Quest::Objective::typeAsString() const {
  const auto typeNames = std::map<Quest::Objective::Type, std::string>{
      {KILL, "kill"}, {FETCH, "fetch"}, {CONSTRUCT, "construct"}};
  auto it = typeNames.find(type);
  if (it == typeNames.end()) return "none";
  return it->second;
}

bool Quest::canBeCompletedByUser(const User& user) const {
  for (const auto& objective : objectives) {
    switch (objective.type) {
      case Objective::NONE:
        assert(false);

      case Objective::FETCH: {
        auto requiredItem = ItemSet{};
        requiredItem.add(objective.item, objective.qty);
        if (!user.hasItems(requiredItem)) return false;
        continue;
      }

      case Objective::KILL:
      case Objective::CONSTRUCT:
        if (user.questProgress(id, objective.type, objective.id) <
            objective.qty)
          return false;
        continue;
    }
  }
  return true;
}
