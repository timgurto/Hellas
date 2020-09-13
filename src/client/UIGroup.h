#pragma once

#include <set>
#include <string>

#include "../combatTypes.h"
#include "../types.h"

class List;
class Client;
class ConfirmationWindow;
#include "ui/Button.h"

struct GroupUI {
  GroupUI(Client& client);
  void clear();
  void refresh();

  void onMembershipChange(const std::set<Username>& upToDateMemberList);
  void addMember(const std::string name);

  void onPlayerLevelChange(Username name, Level newLevel);
  void onPlayerHealthChange(Username name, Hitpoints newHealth);
  void onPlayerEnergyChange(Username name, Energy newEnergy);
  void onPlayerMaxHealthChange(Username name, Hitpoints newMaxHealth);
  void onPlayerMaxEnergyChange(Username name, Energy newMaxEnergy);

  struct Member {
    std::string name;
    std::string level{"?"};
    Hitpoints health{1}, maxHealth{1};
    Energy energy{1}, maxEnergy{1};

    Member(std::string name) : name(name) {}
    bool operator<(const Member& rhs) const { return name < rhs.name; }
  };

  std::set<Member> otherMembers;

  Member* findMember(Username name);

  List* container{nullptr};

  Button* leaveGroupButton{nullptr};
  ConfirmationWindow* leaveGroupConfirmationWindow{nullptr};

  Client& _client;
};
