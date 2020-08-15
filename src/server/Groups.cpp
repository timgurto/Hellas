#include "Groups.h"

#include <cassert>

void Groups::createGroup(User& founder) {
  auto newGroup = Group{};
  newGroup.insert(&founder);
  _groups.push_back(newGroup);
}

void Groups::addToGroup(User& newMember, User& leader) {
  for (auto& group : _groups) {
    if (group.find(&leader) != group.end()) {
      group.insert(&newMember);
      return;
    }
  }
  assert(false);
}

Groups::Group Groups::getMembersOfPlayersGroup(User& player) const {
  for (const auto& group : _groups)
    if (group.find(&player) != group.end()) return group;

  auto soloResult = Group{};
  soloResult.insert(&player);
  return soloResult;
}
