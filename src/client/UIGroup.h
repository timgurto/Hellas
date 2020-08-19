#pragma once

#include <set>
#include <string>

#include "../types.h"

class List;

struct GroupUI {
  void initialise();
  void refresh();
  void addMember(const std::string name);

  struct Member {
    std::string name;
    std::string level{"?"};
    Hitpoints health{1}, maxHealth{1};
    Energy energy{1}, maxEnergy{1};

    Member(std::string name) : name(name) {}
    bool operator<(const Member& rhs) const { return name < rhs.name; }
  };

  std::set<Member> otherMembers;

  List* container{nullptr};
};
