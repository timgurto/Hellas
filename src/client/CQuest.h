#pragma once

#include <map>
#include <string>

class CQuest {
 public:
  using ID = std::string;

  CQuest(const ID &id = "") : _id(id) {}

  bool operator<(const CQuest &rhs) { return _id < rhs._id; }

  const ID &id() const { return _id; }

 private:
  ID _id;
};

using CQuests = std::map<CQuest::ID, CQuest>;
