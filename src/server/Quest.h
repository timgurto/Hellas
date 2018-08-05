#pragma once

#include <map>
#include <string>

class User;
class ServerItem;

struct Quest {
 public:
  using ID = std::string;
  ID id;

  struct Objective {
    enum Type { NONE, KILL, FETCH };
    Type type{NONE};
    void setType(const std::string &asString);

    std::string id{};
    const ServerItem *item{nullptr};
    int qty{1};
  };

  Objective objective;
  bool hasObjective() const { return objective.type != Objective::NONE; }

  bool hasMultipleObjectives{false};

  std::string prerequisiteQuest{};
  bool hasPrerequisite() const { return !prerequisiteQuest.empty(); }
  std::string otherQuestWithThisAsPrerequisite{};
  bool otherQuestHasThisAsPrerequisite() const {
    return !otherQuestWithThisAsPrerequisite.empty();
  }

  bool canBeCompletedByUser(const User &user) const;
};

using Quests = std::map<Quest::ID, Quest>;
