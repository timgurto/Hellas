#pragma once

#include <map>
#include <mutex>
#include <set>
#include <vector>

class User;

class Groups {
 public:
  using Group = std::set<User*>;

  int numGroups() const;

  void createGroup(User& founder);
  void addToGroup(User& newMember, User& inviter);

  Group getMembersOfPlayersGroup(User& aMember) const;
  int getGroupSize(const User& u) const;
  bool isUserInAGroup(const User& u) const;

  void registerInvitation(User& existingMember, User& newMember);
  bool userHasAnInvitation(User& u) const;
  void acceptInvitation(User& newMember);

 private:
  std::vector<Group> _groups;
  std::map<User*, User*> _inviterOf;
  mutable std::mutex _mutex;
};
