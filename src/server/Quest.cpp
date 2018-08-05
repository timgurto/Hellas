#include "Quest.h"
#include "Server.h"
#include "User.h"

void Quest::Objective::setType(const std::string& asString) {
  const auto typesByName = std::map<std::string, Quest::Objective::Type>{
      {"kill", KILL}, {"fetch", FETCH}};
  auto it = typesByName.find(asString);
  if (it == typesByName.end())
    type = NONE;
  else
    type = it->second;
}

bool Quest::canBeCompletedByUser(const User& user) const {
  for (const auto& objective : objectives) {
    switch (objective.type) {
      case Objective::NONE:
        continue;

      case Objective::KILL:
        if (user.killsTowardsQuest(id) < objective.qty) return false;
        continue;

      case Objective::FETCH: {
        auto requiredItem = ItemSet{};
        requiredItem.add(objective.item, objective.qty);
        if (!user.hasItems(requiredItem)) return false;
        continue;
      }
    }
  }
  return true;
}
