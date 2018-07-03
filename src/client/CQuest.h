#pragma once

#include <map>
#include <string>

class CQuest {
 public:
  using ID = std::string;
  using Name = std::string;

  CQuest(const ID &id = "") : _id(id) {}

  bool operator<(const CQuest &rhs) { return _id < rhs._id; }

  const ID &id() const { return _id; }
  const Name &name() const { return _name; }
  void name(const Name &newName) { _name = newName; }

 private:
  ID _id;
  Name _name;
};

using CQuests = std::map<CQuest::ID, CQuest>;
