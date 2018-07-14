#pragma once

#include <map>
#include <string>

struct Quest {
 public:
  using ID = std::string;
  ID id;
  std::string objectiveID{};
  bool hasObjective() const { return !objectiveID.empty(); }
};

using Quests = std::map<Quest::ID, Quest>;
