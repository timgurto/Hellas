#include "UIGroup.h"

#include "Client.h"
#include "ui/ColorBlock.h"
#include "ui/List.h"
#include "ui/ProgressBar.h"

void GroupUI::initialise() {
  const auto SPACE_FOR_MEMBERS = 4;
  const auto GAP = 2_px, MEMBER_W = 100_px, MEMBER_H = 30_px,
             CONTAINER_W = MEMBER_W + 2 * GAP,
             CONTAINER_H = MEMBER_H * SPACE_FOR_MEMBERS,
             CONTAINER_Y = (Client::SCREEN_Y - CONTAINER_H) / 2;
  container = new List({0, CONTAINER_Y, CONTAINER_W, CONTAINER_H}, MEMBER_H);
}

void GroupUI::refresh() {
  container->clearChildren();
  const auto GAP = 2_px, NAME_Y = GAP, NAME_H = Element::TEXT_HEIGHT,
             BAR_H = 6_px, HEALTH_BAR_Y = NAME_Y + NAME_H + GAP,
             ENERGY_BAR_Y = HEALTH_BAR_Y + BAR_H,
             BAR_W = container->contentWidth() - 2 * GAP;

  for (auto member : otherMembers) {
    auto memberEntry = new ColorBlock({});
    container->addChild(memberEntry);

    memberEntry->addChild(
        new Label({GAP, NAME_Y, memberEntry->width(), NAME_H}, member.name));
    auto healthBar = new ProgressBar<Hitpoints>(
        {GAP, HEALTH_BAR_Y, BAR_W, BAR_H}, member.health, member.maxHealth,
        Color::STAT_HEALTH);
    memberEntry->addChild(healthBar);

    auto energyBar = new ProgressBar<Energy>({GAP, ENERGY_BAR_Y, BAR_W, BAR_H},
                                             member.energy, member.maxEnergy,
                                             Color::STAT_ENERGY);
    memberEntry->addChild(energyBar);
  }
}

void GroupUI::addMember(const std::string name) { otherMembers.insert({name}); }
