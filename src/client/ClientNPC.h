#ifndef CLIENT_NPC_H
#define CLIENT_NPC_H

#include "../Point.h"
#include "ClientNPCType.h"
#include "ClientObject.h"

class ClientNPC : public ClientObject {
 public:
  ClientNPC(Client &client, Serial serial, const ClientNPCType *type = nullptr,
            const MapPoint &loc = MapPoint{});
  ~ClientNPC() {}

  const ClientNPCType *npcType() const {
    return dynamic_cast<const ClientNPCType *>(type());
  }

  char classTag() const override { return 'n'; }

  bool canBeTamed() const;
  double getTameChance() const;

  // From ClientCombatant:
  bool canBeAttackedByPlayer() const override;
  Color nameColor() const override;

  // From Sprite:
  void update(double delta) override;
  void draw() const override;
  bool shouldDrawName() const override;

 protected:
  // From ClientObject:
  virtual std::string demolishVerb() const override { return "slaughter"; }
  virtual std::string demolishButtonText() const override {
    return "Slaughter";
  }
  virtual std::string demolishButtonTooltip() const override {
    return "Slaughter this pet";
  }
  bool canBeOwnedByACity() const override { return false; }
  virtual bool addClassSpecificStuffToWindow() override;
};

#endif
