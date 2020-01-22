#include "ClientNPCType.h"

#include "Client.h"
#include "ClientNPC.h"
#include "Surface.h"

ClientNPCType::ClientNPCType(const std::string &id,
                             const std::string &imagePath,
                             Hitpoints maxHealthArg)
    : ClientObjectType(id) {
  maxHealth(maxHealthArg);
  Client &client = *Client::_instance;
  damageParticles(client.findParticleProfile("blood"));

  setImage(imagePath + ".png");
  imageSet(imagePath + ".png");
  corpseImage(imagePath + "-corpse.png");
}

void ClientNPCType::applyTemplate(const CNPCTemplate *nt) {
  collisionRect(nt->collisionRect);
}

void ClientNPCType::addGear(const ClientItem &item) {
  auto slot = item.gearSlot();
  if (_gear.empty()) {
    _gear = {Client::GEAR_SLOTS, {ClientItem::Instance{}, 0}};
  }
  _gear[slot].first = ClientItem::Instance{&item, Item::MAX_HEALTH};
  _gear[slot].second = 1;
}

const ClientItem *ClientNPCType::gear(size_t slot) const {
  if (_gear.empty()) return nullptr;
  return _gear[slot].first.type();
}
