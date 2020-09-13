#include "Stats.h"

#include <iomanip>

#include "util.h"

const Stats &Stats::operator&=(const StatsMod &mod) {
  armor += mod.armor;
  if (armor < 0) armor = 0;

  if (mod.maxHealth < 0 && -mod.maxHealth > static_cast<int>(maxHealth))
    maxHealth = 0;
  else
    maxHealth += mod.maxHealth;

  if (mod.maxEnergy < 0 && -mod.maxEnergy > static_cast<int>(maxEnergy))
    maxEnergy = 0;
  else
    maxEnergy += mod.maxEnergy;

  // Can be negative
  hps += mod.hps;
  eps += mod.eps;

  hit += mod.hit;

  crit += mod.crit;

  critResist += mod.critResist;

  dodge += mod.dodge;

  block += mod.block;

  if (mod.blockValue < 0 && -mod.blockValue > static_cast<int>(blockValue))
    blockValue = 0;
  else
    blockValue += mod.blockValue;

  // Can be negative.
  magicDamage += mod.magicDamage;
  physicalDamage += mod.physicalDamage;
  healing += mod.healing;

  airResist += mod.airResist;
  if (airResist < 0) airResist = 0;

  earthResist += mod.earthResist;
  if (earthResist < 0) earthResist = 0;

  fireResist += mod.fireResist;
  if (fireResist < 0) fireResist = 0;

  waterResist += mod.waterResist;
  if (waterResist < 0) waterResist = 0;

  gatherBonus += mod.gatherBonus;
  if (gatherBonus < 0) gatherBonus = 0;

  // ASSUMPTION: only one item, presumably the weapon, will have this stat.
  if (mod.weaponDamage > 0) weaponDamage = mod.weaponDamage;

  // ASSUMPTION: only one item, presumably the weapon, will have this stat.
  if (mod.attackTime > 0) attackTime = mod.attackTime;

  followerLimit += mod.followerLimit;
  if (followerLimit < 0) followerLimit = 0;

  if (mod.speed < 0) speed = 0;
  if (mod.speed != 1.0) speed *= mod.speed;

  stunned = stunned || mod.stuns;

  return *this;
}

Stats Stats::operator&(const StatsMod &mod) const {
  Stats stats = *this;
  stats &= mod;
  return stats;
}

ArmourClass Stats::resistanceByType(SpellSchool school) const {
  if (school == SpellSchool::PHYSICAL) return armor;
  if (school == SpellSchool::AIR) return airResist;
  if (school == SpellSchool::EARTH) return earthResist;
  if (school == SpellSchool::FIRE) return fireResist;
  if (school == SpellSchool::WATER) return waterResist;

  // Can't report, as this could be the server or the client.
  return armor;
}

std::vector<std::string> StatsMod::toStrings() const {
  auto v = std::vector<std::string>{};
  if (attackTime > 0) {
    auto inSeconds = attackTime / 1000.0;
    auto formatted = std::ostringstream{};
    formatted << std::setprecision(1) << std::fixed << inSeconds << "s speed"s;
    v.push_back(formatted.str());
  }
  if (weaponDamage > 0) {
    std::string schoolString =
        weaponSchool == SpellSchool::PHYSICAL ? ""s : std::string{weaponSchool};
    auto line = toString(weaponDamage) + " "s + schoolString + " damage"s;
    if (attackTime > 0) {
      auto dps = std::ostringstream{};
      dps << std::setprecision(2) << std::fixed
          << 1000.0 * weaponDamage / attackTime;
      line += " ("s + dps.str() + " per second)"s;
    }
    v.push_back(line);
  }
  if (armor > 0) v.push_back("+" + toString(armor) + " armour");
  if (maxHealth > 0) v.push_back("+" + toString(maxHealth) + " max health");
  if (maxEnergy > 0) v.push_back("+" + toString(maxEnergy) + " max energy");
  if (hps > 0) v.push_back("+" + toString(hps) + " health per second");
  if (eps > 0)
    v.push_back("+" + toString(eps) + " energy per second");
  else if (eps < 0)
    v.push_back("-" + toString(-eps) + " energy per second");
  if (hit) v.push_back("+" + toString(hit) + "% hit");
  if (crit) v.push_back("+" + toString(crit) + "% crit");
  if (critResist)
    v.push_back("-" + toString(critResist) + "% chance to be crit");
  if (dodge) v.push_back("+" + toString(dodge) + "% dodge");
  if (block) v.push_back("+" + toString(block) + "% block");
  if (blockValue > 0) v.push_back("+" + toString(blockValue) + " block value");
  if (magicDamage > 0)
    v.push_back("+" + toString(magicDamage) + " magic damage");
  if (physicalDamage > 0)
    v.push_back("+" + toString(physicalDamage) + " physical damage");
  if (healing > 0)
    v.push_back("+" + toString(healing) + " healing-spell amount");
  if (airResist > 0) v.push_back("+" + toString(airResist) + " air resistance");
  if (earthResist > 0)
    v.push_back("+" + toString(earthResist) + " earth resistance");
  if (fireResist > 0)
    v.push_back("+" + toString(fireResist) + " fire resistance");
  if (waterResist > 0)
    v.push_back("+" + toString(waterResist) + " water resistance");
  if (gatherBonus > 0)
    v.push_back("+" + toString(gatherBonus) + "% chance to gather double");
  if (followerLimit > 0)
    v.push_back("+" + toString(followerLimit) + " max. following pets");
  if (speed != 1.0)
    v.push_back("+" + multiplicativeToString(speed) + " run speed");

  return v;
}

std::string StatsMod::buffDescription() const {
  if (stuns) return "Stun "s;

  auto ret = "Grant "s;
  auto statStrings = toStrings();
  auto firstStringHasBeenPrinted = false;
  for (const auto &statString : statStrings) {
    if (firstStringHasBeenPrinted) ret += ", ";
    ret += statString;
    firstStringHasBeenPrinted = true;
  }
  ret += " to "s;
  return ret;
}
