#include "ClientNPCType.h"

#include "Client.h"
#include "ClientNPC.h"
#include "Surface.h"

ClientNPCType::ClientNPCType(const std::string &id,
                             const std::string &imagePath,
                             Hitpoints maxHealthArg, const Client &client)
    : ClientObjectType(id) {
  maxHealth(maxHealthArg);
  damageParticles(client.findParticleProfile("blood"));

  setImage(imagePath);
  _corpseImage = {imagePath + "-corpse"};
}

void ClientNPCType::applyTemplate(const CNPCTemplate *nt,
                                  const Client &client) {
  collisionRect(nt->collisionRect);
  setSoundProfile(client, nt->soundProfile);
}

void ClientNPCType::addGear(const ClientItem &item) {
  auto slot = item.gearSlot();
  if (_gear.empty()) {
    _gear = {Client::GEAR_SLOTS, {ClientItem::Instance{}, 0}};
  }
  _gear[slot].first = ClientItem::Instance{&item, Item::MAX_HEALTH, false};
  _gear[slot].second = 1;
}

const ClientItem *ClientNPCType::gear(size_t slot) const {
  if (_gear.empty()) return nullptr;
  return _gear[slot].first.type();
}
