#include "Groups.h"

#include <cassert>

#include "User.h"

Groups::~Groups() {
  for (auto* group : _groups) delete group;
}

int Groups::numGroups() const { return _groups.size(); }

Groups::Group* Groups::getGroupAndMakeIfNeeded(User& inviter) {
  auto it = _groupsByUser.find(&inviter);
  auto inviterIsInAGroup = it != _groupsByUser.end();

  if (inviterIsInAGroup)
    return it->second;
  else {
    return createGroup(inviter);
  }
}

Groups::Group* Groups::createGroup(User& founder) {
  auto newGroup = new Group;
  newGroup->insert(&founder);
  _groupsByUser[&founder] = newGroup;
  _groups.push_back(newGroup);
  return newGroup;
}

void Groups::inviteToGroup(User& newMember, User& inviter) {
  auto* group = getGroupAndMakeIfNeeded(inviter);

  group->insert(&newMember);
  _groupsByUser[&newMember] = group;
  sendGroupMakeupToAllMembers(*group);
}

Groups::Group Groups::getUsersGroup(User& player) const {
  auto it = _groupsByUser.find(&player);
  auto userIsInAGroup = it != _groupsByUser.end();

  if (userIsInAGroup) return *it->second;

  auto soloResult = Group{&player};
  return soloResult;
}

int Groups::getGroupSize(const User& u) const {
  auto it = _groupsByUser.find(&u);
  auto userIsInAGroup = it != _groupsByUser.end();

  if (userIsInAGroup)
    return it->second->size();
  else
    return 1;
}

bool Groups::isUserInAGroup(const User& u) const {
  return _groupsByUser.count(&u) == 1;
}

void Groups::registerInvitation(User& existingMember, User& newMember) {
  _inviterOf[&newMember] = &existingMember;
}

bool Groups::userHasAnInvitation(User& u) const {
  return _inviterOf.count(&u) > 0;
}

void Groups::acceptInvitation(User& newMember) {
  auto& inviter = *_inviterOf[&newMember];
  inviteToGroup(newMember, inviter);
}

void Groups::sendGroupMakeupToAllMembers(const Group& g) {
  for (const auto* member : g) {
    sendGroupMakeupTo(g, *member);
  }
}

void Groups::sendGroupMakeupTo(const Group& g, const User& recipient) {
  auto args = makeArgs(g.size() - 1);

  for (const auto* member : g) {
    if (member == &recipient) continue;
    args = makeArgs(args, member->name());
  }

  recipient.sendMessage({SV_GROUPMATES, args});
}
