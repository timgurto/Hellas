#pragma once

#include <map>
#include <string>

class User;
class ServerItem;

struct Quest {
 public:
  using ID = std::string;
  ID id;
  int objectiveQty{1};

  enum ObjectiveType { NONE, KILL, FETCH };
  ObjectiveType objectiveType{NONE};
  std::string objectiveID{};
  const ServerItem *itemObjective{nullptr};
  void setObjectiveType(const std::string &asString);
  bool hasObjective() const { return objectiveType != NONE; }

  std::string prerequisiteQuest{};
  bool hasPrerequisite() const { return !prerequisiteQuest.empty(); }
  std::string otherQuestWithThisAsPrerequisite{};
  bool otherQuestHasThisAsPrerequisite() const {
    return !otherQuestWithThisAsPrerequisite.empty();
  }

  bool canBeCompletedByUser(const User &user) const;
};

using Quests = std::map<Quest::ID, Quest>;
