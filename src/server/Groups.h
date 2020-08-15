#pragma once

#include "User.h"

class Groups {
 public:
  void createGroup(User& founder) { _members.insert(&founder); }
  void addToGroup(User& newMember, const User& leader) {
    _members.insert(&newMember);
  }

  std::set<User*>& members() { return _members; }
  bool aGroupExists() const { return _members.size() > 0; }

 private:
  std::set<User*> _members;
};
