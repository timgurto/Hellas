#pragma once

#include <map>
#include <string>

struct Quest {
 public:
  using ID = std::string;
  ID id;
  bool hasObjective{false};
};

using Quests = std::map<Quest::ID, Quest>;
