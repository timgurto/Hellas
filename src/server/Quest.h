#pragma once

#include <map>
#include <string>

struct Quest {
 public:
  using ID = std::string;
  ID id;
  std::string objectiveID{};
  bool hasObjective() const { return !objectiveID.empty(); }
  std::string prerequisiteQuest{};
  bool hasPrerequisite() const { return !prerequisiteQuest.empty(); }
  std::string otherQuestWithThisAsPrerequisite{};
  bool otherQuestHasThisAsPrerequisite() const {
    return !otherQuestWithThisAsPrerequisite.empty();
  }
};

using Quests = std::map<Quest::ID, Quest>;
