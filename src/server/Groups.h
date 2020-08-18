#pragma once

#include <map>
#include <mutex>
#include <set>
#include <vector>

class User;

class Groups {
 public:
  ~Groups();

  using Group = std::set<User*>;

  int numGroups() const;

  void createGroup(User& founder);
  void addToGroup(User& newMember, User& inviter);

  Group getUsersGroup(User& aMember) const;
  int getGroupSize(const User& u) const;
  bool isUserInAGroup(const User& u) const;

  void registerInvitation(User& existingMember, User& newMember);
  bool userHasAnInvitation(User& u) const;
  void acceptInvitation(User& newMember);

  static void sendGroupMakeupToAllMembers(const Group& g);
  static void sendGroupMakeupTo(const Group& g, const User& recipient);

 private:
  std::vector<Group*> _groups;
  std::map<User*, User*> _inviterOf;
  mutable std::mutex _mutex;
};
