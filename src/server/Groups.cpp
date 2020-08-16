#include "Groups.h"

#include <cassert>

int Groups::numGroups() const { return _groups.size(); }

void Groups::createGroup(User& founder) {
  auto newGroup = Group{};
  newGroup.insert(&founder);
  _mutex.lock();
  _groups.push_back(newGroup);
  _mutex.unlock();
}

void Groups::addToGroup(User& newMember, User& inviter) {
  _mutex.lock();
  for (auto& group : _groups) {
    if (group.find(&inviter) != group.end()) {
      group.insert(&newMember);
      _mutex.unlock();
      return;
    }
  }
  _mutex.unlock();
  assert(false);
}

Groups::Group Groups::getMembersOfPlayersGroup(User& player) const {
  _mutex.lock();
  for (const auto& group : _groups)
    if (group.find(&player) != group.end()) {
      _mutex.unlock();
      return group;
    }
  _mutex.unlock();

  auto soloResult = Group{};
  soloResult.insert(&player);
  return soloResult;
}

int Groups::getGroupSize(const User& u) const {
  _mutex.lock();
  for (const auto& group : _groups) {
    auto* nonConstUser = const_cast<User*>(&u);
    if (group.find(nonConstUser) != group.end()) {
      _mutex.unlock();
      return group.size();
    }
  }
  _mutex.unlock();
  return 1;
}

bool Groups::isUserInAGroup(const User& u) const {
  _mutex.lock();
  for (const auto& group : _groups) {
    auto* nonConstUser = const_cast<User*>(&u);
    if (group.find(nonConstUser) != group.end()) {
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
  addToGroup(newMember, inviter);
}
