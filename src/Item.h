#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <vector>

#include "HasTags.h"
#include "Podes.h"
#include "Stats.h"

struct ItemClass;

class Item : public HasTags {
 public:
  enum GearSlot {
    HEAD = 0,
    JEWELRY = 1,
    BODY = 2,
    SHOULDERS = 3,
    HANDS = 4,
    FEET = 5,
    WEAPON = 6,
    OFFHAND = 7,

    NOT_GEAR
  };
  enum Quality { COMMON = 1, UNCOMMON = 2, RARE = 3, EPIC = 4, LEGENDARY = 5 };
  enum Soulbinding { NO_BINDING, BIND_ON_PICKUP, BIND_ON_EQUIP };

  Item(const std::string &id);
  virtual ~Item() {}

  const std::string &id() const { return _id; }
  void gearSlot(GearSlot slot) { _gearSlot = slot; }
  static GearSlot parseGearSlot(std::string slotName);
  GearSlot gearSlot() const { return _gearSlot; }
  bool isGear() const { return _gearSlot != Item::NOT_GEAR; }
  void stats(const StatsMod &stats) { _stats = stats; }
  const StatsMod &stats() const { return _stats; }
  void useSuffixSet(std::string suffixSetID) { _suffixSet = suffixSetID; }
  bool hasSuffix() const { return !_suffixSet.empty(); }
  void makeWeapon(Hitpoints damage, double speedInS, SpellSchool school);
  void weaponRange(Podes range) { _weaponRange = range.toPixels(); }
  px_t weaponRange() const { return _weaponRange; }
  void weaponAmmo(const std::string &id) { _weaponAmmoID = id; }
  const Item *weaponAmmo() const { return _weaponAmmo; }
  bool usesAmmo() const { return _weaponAmmo != nullptr; }
  void castsSpellOnUse(const std::string &spell, const std::string &arg) {
    _castsSpellOnUse = spell;
    _spellArg = arg;
  }
  bool castsSpellOnUse() const { return !_castsSpellOnUse.empty(); }
  const std::string &spellArg() const { return _spellArg; }
  const std::string &spellToCastOnUse() const { return _castsSpellOnUse; }
  Level lvlReq() const { return _lvlReq; }
  bool hasLvlReq() const { return _lvlReq > 0; }
  void setBinding(std::string mode);
  bool bindsOnPickup() const { return _soulbinding == BIND_ON_PICKUP; }
  bool bindsOnEquip() const { return _soulbinding == BIND_ON_EQUIP; }
  void setClass(const ItemClass &itemClass) { _class = &itemClass; }
  const ItemClass *getClass() const { return _class; }
  Level ilvl() const { return _ilvl; }
  void ilvl(Level l) { _ilvl = l; }
  Hitpoints maxHealth() const { return _maxHealth; }
  void quality(int q) { _quality = static_cast<Quality>(q); }
  Quality quality() const { return _quality; }
  void initialiseLvlReq();
  void initialiseMaxHealthFromIlvlAndQuality();
  bool canBeDamaged() const;

  bool operator<(const Item &rhs) const { return _id < rhs._id; }

  typedef std::vector<std::pair<const Item *, size_t> > vect_t;

  static GearSlot getRandomArmorSlot();

  virtual void fetchAmmoItem() const = 0;

 protected:
  std::string _id;  // The no-space, unique name used in data files
  GearSlot _gearSlot = NOT_GEAR;
  Level _lvlReq{0};
  StatsMod _stats;  // If gear, the impact it has on its wearer's stats.
  std::string _suffixSet;
  Soulbinding _soulbinding{NO_BINDING};
  Level _ilvl{0};
  Hitpoints _maxHealth{1};
  Quality _quality{COMMON};

  const ItemClass *_class{nullptr};

  // If a weapon, how close the holder must be to a target to it.
  px_t _weaponRange = Podes::MELEE_RANGE.toPixels();
  std::string _weaponAmmoID{};
  mutable const Item *_weaponAmmo{nullptr};  // Fetched in fetchAmmoItem()

 private:
  std::string _castsSpellOnUse{};
  std::string _spellArg{};
};

#endif
