#pragma once

#include <set>
#include <vector>

class User;

class Groups {
 public:
  using Group = std::set<User*>;

  void createGroup(User& founder);
  void addToGroup(User& newMember, User& leader);

  Group getMembersOfPlayersGroup(User& aMember) const;
  int getGroupSize(const User& u) const;

 private:
  std::vector<Group> _groups;
};
