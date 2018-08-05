#include "Quest.h"
#include "Server.h"
#include "User.h"

void Quest::setObjectiveType(const std::string& asString) {
  const auto typesByName = std::map<std::string, Quest::ObjectiveType>{
      {"kill", Quest::KILL}, {"fetch", Quest::FETCH}};
  auto it = typesByName.find(asString);
  if (it == typesByName.end())
    objectiveType = NONE;
  else
    objectiveType = it->second;
}

bool Quest::canBeCompletedByUser(const User& user) const {
  if (hasMultipleObjectives) return false;

  switch (objectiveType) {
    case NONE:
      return true;
    case KILL:
      return user.killsTowardsQuest(id) >= objectiveQty;
    case FETCH: {
      auto requiredItem = ItemSet{};
      requiredItem.add(itemObjective, objectiveQty);
      return user.hasItems(requiredItem);
    }
  }

  return false;
}
