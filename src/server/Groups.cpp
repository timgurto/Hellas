#include "Groups.h"

#include <cassert>

#include "User.h"

Groups::~Groups() {
  for (auto* group : _groups) delete group;
}

int Groups::numGroups() const { return _groups.size(); }

void Groups::createGroup(User& founder) {
  auto newGroup = new Group;
  newGroup->insert(&founder);
  _groupsByUser[&founder] = newGroup;
  _mutex.lock();
  _groups.push_back(newGroup);
  _mutex.unlock();
}

  _mutex.lock();
  for (auto* group : _groups) {
    if (group->find(&inviter) != group->end()) {
      group->insert(&newMember);
      _groupsByUser[&newMember] = group;
      sendGroupMakeupToAllMembers(*group);
      _mutex.unlock();
      return;
    }
  }
  _mutex.unlock();
  assert(false);
void Groups::inviteToGroup(User& newMember, User& inviter) {
}

Groups::Group Groups::getUsersGroup(User& player) const {
  _mutex.lock();
  auto it = _groupsByUser.find(&player);
  _mutex.unlock();
  auto userIsInAGroup = it != _groupsByUser.end();

  if (userIsInAGroup) return *it->second;

  auto soloResult = Group{&player};
  return soloResult;
}

int Groups::getGroupSize(const User& u) const {
  _mutex.lock();
  for (const auto* group : _groups) {
    auto* nonConstUser = const_cast<User*>(&u);
    if (group->find(nonConstUser) != group->end()) {
      _mutex.unlock();
      return group->size();
    }
  }
  _mutex.unlock();
  return 1;
}

bool Groups::isUserInAGroup(const User& u) const {
  _mutex.lock();
  for (const auto* group : _groups) {
    auto* nonConstUser = const_cast<User*>(&u);
    if (group->find(nonConstUser) != group->end()) {
      _mutex.unlock();
      return true;
    }
  }
  _mutex.unlock();
  return false;
}

void Groups::registerInvitation(User& existingMember, User& newMember) {
  _inviterOf[&newMember] = &existingMember;
}

bool Groups::userHasAnInvitation(User& u) const {
  return _inviterOf.count(&u) > 0;
}

void Groups::acceptInvitation(User& newMember) {
  auto& inviter = *_inviterOf[&newMember];
  if (!isUserInAGroup(inviter)) createGroup(inviter);
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
