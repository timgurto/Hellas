#pragma once

#include <map>
#include <string>

class CQuest {
 public:
  using ID = std::string;

 private:
  ID _id;
};

using CQuests = std::map<CQuest::ID, CQuest>;
