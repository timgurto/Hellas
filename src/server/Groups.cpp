#include "Groups.h"

#include <cassert>

#include "Server.h"
#include "User.h"

Groups::~Groups() {
  for (auto* group : _groups) delete group;
}

int Groups::numGroups() const { return _groups.size(); }

Groups::Group* Groups::getGroupAndMakeIfNeeded(Username inviter) {
  auto it = _groupsByUser.find(inviter);
  auto inviterIsInAGroup = it != _groupsByUser.end();

  if (inviterIsInAGroup)
    return it->second;
  else {
    return createGroup(inviter);
  }
}

Groups::Group* Groups::createGroup(Username founder) {
  auto newGroup = new Group;
  newGroup->insert(founder);
  _groupsByUser[founder] = newGroup;
  _groups.push_back(newGroup);
  return newGroup;
}

void Groups::addToGroup(Username newMember, Username inviter) {
  auto* group = getGroupAndMakeIfNeeded(inviter);

  group->insert(newMember);
  _groupsByUser[newMember] = group;
  sendGroupMakeupToAllMembers(*group);
}

Groups::Group Groups::getUsersGroup(Username player) const {
  auto it = _groupsByUser.find(player);
  auto userIsInAGroup = it != _groupsByUser.end();

  if (userIsInAGroup) return *it->second;

  auto soloResult = Group{player};
  return soloResult;
}

int Groups::getGroupSize(Username u) const {
  auto it = _groupsByUser.find(u);
  auto userIsInAGroup = it != _groupsByUser.end();

  if (userIsInAGroup)
    return it->second->size();
  else
    return 1;
}

bool Groups::isUserInAGroup(Username u) const {
  return _groupsByUser.count(u) == 1;
}

bool Groups::areUsersInSameGroup(Username lhs, Username rhs) const {
  auto lhsGroup = getUsersGroup(lhs);
  return lhsGroup.count(rhs) == 1;
}

void Groups::registerInvitation(Username existingMember, Username newMember) {
  _inviterOf[newMember] = existingMember;
}

bool Groups::userHasAnInvitation(Username u) const {
  return _inviterOf.count(u) == 1;
}

void Groups::acceptInvitation(Username newMember) {
  auto inviter = _inviterOf[newMember];
  addToGroup(newMember, inviter);
}

void Groups::removeUserFromHisGroup(Username quitter) {
  _groupsByUser.erase(quitter);
}

void Groups::sendGroupMakeupToAllMembers(const Group& g) {
  for (auto memberName : g) {
    auto* asUser = Server::instance().getUserByName(memberName);
    if (asUser) sendGroupMakeupTo(g, *asUser);
  }
}

void Groups::sendGroupMakeupTo(const Group& g, const User& recipient) {
  auto args = makeArgs(g.size() - 1);

  for (auto memberName : g) {
    if (memberName == recipient.name()) continue;
    args = makeArgs(args, memberName);
  }

  recipient.sendMessage({SV_GROUPMATES, args});
}
