#ifndef ITEM_H
#define ITEM_H

#include <set>
#include <string>
#include <vector>

#include "Podes.h"
#include "Stats.h"

class Item {
 public:
  static const size_t WEAPON_SLOT = 6, OFFHAND_SLOT = 7;
  static const ItemHealth MAX_HEALTH = 100;

  Item(const std::string &id);
  virtual ~Item() {}

  const std::string &id() const { return _id; }
  const std::set<std::string> &tags() const { return _tags; }
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

  bool operator<(const Item &rhs) const { return _id < rhs._id; }

  typedef std::vector<std::pair<const Item *, size_t> > vect_t;

  void addTag(const std::string &tagName);
  bool hasTags() const { return _tags.size() > 0; }
  bool isTag(const std::string &tagName) const;

  static size_t getRandomArmorSlot();

  virtual void fetchAmmoItem() const = 0;

 protected:
  std::string _id;  // The no-space, unique name used in data files
  std::set<std::string> _tags;
  size_t _gearSlot;
  StatsMod _stats;  // If gear, the impact it has on its wearer's stats.

  // If a weapon, how close the holder must be to a target to it.
  px_t _weaponRange = Podes::MELEE_RANGE.toPixels();
  std::string _weaponAmmoID{};
  mutable const Item *_weaponAmmo{nullptr};  // Fetched in fetchAmmoItem()

 private:
  Hitpoints _durability;

  std::string _castsSpellOnUse{};
};

#endif
