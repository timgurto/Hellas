#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <vector>

#include "HasTags.h"
#include "Podes.h"
#include "Stats.h"

class Item : public HasTags {
 public:
  static const size_t WEAPON_SLOT = 6, OFFHAND_SLOT = 7;
  static const Hitpoints MAX_HEALTH = 100;

  Item(const std::string &id);
  virtual ~Item() {}

  const std::string &id() const { return _id; }
  void gearSlot(size_t slot) { _gearSlot = slot; }
  size_t gearSlot() const { return _gearSlot; }
  void stats(const StatsMod &stats) { _stats = stats; }
  const StatsMod &stats() const { return _stats; }
  Hitpoints durability() const { return _durability; }
  void durability(Hitpoints n) { _durability = n; }
  void makeWeapon(Hitpoints damage, double speedInS, SpellSchool school);
  void weaponRange(Podes range) { _weaponRange = range.toPixels(); }
  px_t weaponRange() const { return _weaponRange; }
  void weaponAmmo(const std::string &id) { _weaponAmmoID = id; }
  const Item *weaponAmmo() const { return _weaponAmmo; }
  bool usesAmmo() const { return _weaponAmmo != nullptr; }
  void castsSpellOnUse(const std::string &spell) { _castsSpellOnUse = spell; }
  bool castsSpellOnUse() const { return !_castsSpellOnUse.empty(); }
  const std::string &spellToCastOnUse() const { return _castsSpellOnUse; }
  void makeRepairable() { _repairInfo.canBeRepaired = true; }
  void repairingCosts(const std::string &costID) { _repairInfo.cost = costID; }
  void repairingRequiresTool(const std::string &tag) { _repairInfo.tool = tag; }
  RepairInfo repairInfo() const { return _repairInfo; }
  void lvlReq(Level req) { _lvlReq = req; }
  bool hasLvlReq() const { return _lvlReq > 0; }

  bool operator<(const Item &rhs) const { return _id < rhs._id; }

  typedef std::vector<std::pair<const Item *, size_t> > vect_t;

  static size_t getRandomArmorSlot();

  virtual void fetchAmmoItem() const = 0;

 protected:
  std::string _id;  // The no-space, unique name used in data files
  size_t _gearSlot;
  Level _lvlReq{0};
  StatsMod _stats;  // If gear, the impact it has on its wearer's stats.

  RepairInfo _repairInfo;

  // If a weapon, how close the holder must be to a target to it.
  px_t _weaponRange = Podes::MELEE_RANGE.toPixels();
  std::string _weaponAmmoID{};
  mutable const Item *_weaponAmmo{nullptr};  // Fetched in fetchAmmoItem()

 private:
  Hitpoints _durability;

  std::string _castsSpellOnUse{};
};

#endif
