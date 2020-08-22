#pragma once

#ifdef TESTING
#include <mutex>
#endif

#include <map>
#include <set>
#include <vector>

#include "../types.h"

class User;

class Groups {
 public:
  using Group = std::set<Username>;

  int numGroups() const { return _numGroups; }

  void addToGroup(Username newMember, Username inviter);

  Group getUsersGroup(Username aMember) const;
  int getGroupSize(Username u) const;
  bool isUserInAGroup(Username u) const;
  bool areUsersInSameGroup(Username lhs, Username rhs) const;

  void registerInvitation(Username existingMember, Username newMember);
  bool userHasAnInvitation(Username u) const;
  void acceptInvitation(Username newMember);

  void removeUserFromHisGroup(Username quitter);

  static void sendGroupMakeupToAllMembers(const Group& g);
  static void sendGroupMakeupTo(const Group& g, const User& recipient);

 private:
  Group* getGroupAndMakeIfNeeded(Username inviter);
  Group* createGroup(Username founder);
  std::map<Username, Username> _inviterOf;
  std::map<Username, Group*> _groupsByUser;
#ifdef TESTING
  mutable std::mutex _groupsByUserMutex;
#endif
  int _numGroups{0};
};
