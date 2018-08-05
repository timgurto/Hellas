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
  if (hasMultipleObjectives) return false;

  switch (objective.type) {
    case Objective::NONE:
      return true;
    case Objective::KILL:
      return user.killsTowardsQuest(id) >= objective.qty;
    case Objective::FETCH: {
      auto requiredItem = ItemSet{};
      requiredItem.add(objective.item, objective.qty);
      return user.hasItems(requiredItem);
    }
  }

  return false;
}
