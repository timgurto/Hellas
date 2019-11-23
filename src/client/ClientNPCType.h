#ifndef CLIENT_NPC_TYPE_H
#define CLIENT_NPC_TYPE_H

#include "ClientObject.h"
#include "Projectile.h"

class SoundProfile;

class ClientNPCType : public ClientObjectType {
 public:
  ClientNPCType(const std::string &id, const std::string &imagePath,
                Hitpoints maxHealth);
  virtual ~ClientNPCType() override {}

  void projectile(const Projectile::Type &type) { _projectile = &type; }
  const Projectile::Type *projectile() const { return _projectile; }

  void addGear(const ClientItem &item);
  const ClientItem *gear(size_t slot) const;
  const ClientItem::vect_t &gear() const { return _gear; }
  bool hasGear() const { return !_gear.empty(); }

  void makeCivilian() { _isCivilian = true; }
  bool isCivilian() const { return _isCivilian; }
  void makeNeutral() { _isNeutral = true; }
  bool isNeutral() const { return _isNeutral; }
  void canBeTamed(bool b) { _canBeTamed = b; }
  bool canBeTamed() const { return _canBeTamed; }

  virtual char classTag() const override { return 'n'; }

 private:
  const Projectile::Type *_projectile = nullptr;
  ClientItem::vect_t _gear;  // For humanoid NPCs
  bool _isCivilian{false};
  bool _isNeutral{false};
  bool _canBeTamed{false};
};

#endif
