#include "UIGroup.h"

#include "Client.h"
#include "ui/ColorBlock.h"
#include "ui/List.h"
#include "ui/ProgressBar.h"

GroupUI::GroupUI(Client &client) : _client(client) {
  const auto GAP = 2_px, MEMBER_W = 100_px, MEMBER_H = 30_px,
             CONTAINER_W = MEMBER_W + 2 * GAP,
             CONTAINER_Y = (Client::SCREEN_Y) / 2;
  container = new List({0, CONTAINER_Y, CONTAINER_W, 0}, MEMBER_H);
}

void GroupUI::refresh() {
  container->clearChildren();
  const auto GAP = 2_px, NAME_H = Element::TEXT_HEIGHT, LEVEL_W = 15,
             BAR_H = 6_px, BAR_W = container->contentWidth() - 2 * GAP;

  for (auto &member : otherMembers) {
    auto memberEntry = new ColorBlock({});
    container->addChild(memberEntry);

    auto y = GAP;

    auto x = GAP;
    memberEntry->addChild(new Label({x, y, LEVEL_W, NAME_H}, member.level));
    x += LEVEL_W;
    memberEntry->addChild(
        new Label({x, y, memberEntry->width(), NAME_H}, member.name));
    y += NAME_H + GAP;

    auto healthBar =
        new ProgressBar<Hitpoints>({GAP, y, BAR_W, BAR_H}, member.health,
                                   member.maxHealth, Color::STAT_HEALTH);
    memberEntry->addChild(healthBar);
    y += BAR_H;

    auto energyBar =
        new ProgressBar<Energy>({GAP, y, BAR_W, BAR_H}, member.energy,
                                member.maxEnergy, Color::STAT_ENERGY);
    memberEntry->addChild(energyBar);

    auto targetName = member.name;
    memberEntry->setLeftMouseDownFunction(
        [targetName, this](Element &, const ScreenPoint &) {
          auto *avatar = _client.findUser(targetName);
          if (avatar) _client.setTarget(*avatar);
        });

    auto playerIsOnline = _client.findUser(targetName) != nullptr;
    if (!playerIsOnline) memberEntry->setAlpha(0x7f);
  }

  container->resizeToContent();
  auto midScreenHeight = (Client::SCREEN_Y - container->height()) / 2;
  container->setPosition(0, midScreenHeight);
}

void GroupUI::addMember(const std::string name) {
  auto m = Member{name};
  auto *user = _client.findUser(name);
  if (user) {
    m.level = toString(user->level());
    m.health = user->health();
    m.maxHealth = user->maxHealth();
    m.energy = user->energy();
    m.maxEnergy = user->maxEnergy();
  }
  otherMembers.insert(m);
}

void GroupUI::onPlayerLevelChange(Username name, Level newLevel) {
  auto *member = findMember(name);
  if (member) member->level = toString(newLevel);
}

void GroupUI::onPlayerHealthChange(Username name, Hitpoints newHealth) {
  auto *member = findMember(name);
  if (member) member->health = newHealth;
}

void GroupUI::onPlayerEnergyChange(Username name, Energy newEnergy) {
  auto *member = findMember(name);
  if (member) member->energy = newEnergy;
}

void GroupUI::onPlayerMaxHealthChange(Username name, Hitpoints newMaxHealth) {
  auto *member = findMember(name);
  if (member) member->maxHealth = newMaxHealth;
}

void GroupUI::onPlayerMaxEnergyChange(Username name, Energy newMaxEnergy) {
  auto *member = findMember(name);
  if (member) member->maxEnergy = newMaxEnergy;
}

GroupUI::Member *GroupUI::findMember(Username name) {
  auto it = otherMembers.find({name});
  if (it == otherMembers.end()) return nullptr;

  // const_cast is fine as long as .name never changes, since it's used to sort
  // the set.
  return const_cast<Member *>(&*it);
}
