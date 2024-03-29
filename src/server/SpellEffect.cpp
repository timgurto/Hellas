#include "SpellEffect.h"

#include "Server.h"

void SpellEffect::setFunction(const std::string &functionName) {
  auto it = functionMap.find(functionName);
  if (it != functionMap.end()) _function = it->second;
}

SpellEffect::FunctionMap SpellEffect::functionMap = {
    {"doDirectDamage", doDirectDamage},
    {"doDirectDamageWithModifiedThreat", doDirectDamageWithModifiedThreat},
    {"heal", heal},
    {"buff", buff},
    {"debuff", debuff},
    {"scaleThreat", scaleThreat},
    {"dispellDebuff", dispellDebuff},
    {"randomTeleport", randomTeleport},
    {"teleportToCity", teleportToCity},
    {"teleportToHouse", teleportToHouse},
    {"teachRecipe", teachRecipe},
    {"spawnNPC", spawnNPC},
    {"randomBuff", randomBuff},
    {"playTwoUp", playTwoUp}};

SpellEffect::FlagMap SpellEffect::aggressionMap = {
    {doDirectDamage, true}, {doDirectDamageWithModifiedThreat, true},
    {heal, false},          {buff, false},
    {debuff, true},         {scaleThreat, false},
    {dispellDebuff, false}};

bool SpellEffect::isAggressive() const {
  if (!exists()) return false;
  auto it = aggressionMap.find(_function);
  if (it == aggressionMap.end()) return false;
  auto isThisEffectAggressive = it->second;
  return isThisEffectAggressive;
}

CombatResult SpellEffect::doDirectDamage(const SpellEffect &effect,
                                         Entity &caster, Entity &target,
                                         const std::string &supplementaryArg) {
  auto effectWithDefaultThreat = effect;
  effectWithDefaultThreat._args.d1 = 1.0;  // Threat
  return doDirectDamageWithModifiedThreat(effectWithDefaultThreat, caster,
                                          target, supplementaryArg);
}

CombatResult SpellEffect::doDirectDamageWithModifiedThreat(
    const SpellEffect &effect, Entity &caster, Entity &target,
    const std::string &supplementaryArg) {
  const auto outcome =
      caster.generateHitAgainst(target, DAMAGE, effect._school, effect._range);
  if (outcome == MISS || outcome == DODGE) return outcome;

  auto rawDamage = static_cast<double>(effect._args.i1);
  if (effect.school() == SpellSchool::PHYSICAL)
    rawDamage = caster.stats().physicalDamage.addTo(rawDamage);
  else
    rawDamage = caster.stats().magicDamage.addTo(rawDamage);

  if (outcome == CRIT) rawDamage *= 2;

  auto resistance = target.stats().resistanceByType(effect._school);
  resistance = resistance.modifyByLevelDiff(caster.level(), target.level());
  rawDamage = resistance.applyTo(rawDamage);

  auto damage = chooseRandomSpellMagnitude(rawDamage);

  if (outcome == BLOCK) {
    auto blockAmount = target.stats().blockValue.effectiveValue();
    if (blockAmount >= damage)
      damage = 0;
    else
      damage -= blockAmount;
  }

  // onAttackedBy() tags the target, and therefore must be called before the
  // potentially lethal damage is done.
  auto threatScaler = effect._args.d1;
  auto threat = toInt(damage * threatScaler);
  target.onAttackedBy(caster, threat, outcome);

  target.reduceHealth(damage);

  return outcome;
}

CombatResult SpellEffect::heal(const SpellEffect &effect, Entity &caster,
                               Entity &target,
                               const std::string &supplementaryArg) {
  if (!target.canBeHealedBySpell()) return FAIL;

  auto outcome =
      caster.generateHitAgainst(target, HEAL, effect._school, effect._range);

  auto rawAmountToHeal = static_cast<double>(effect._args.i1);
  rawAmountToHeal = caster.stats().healing.addTo(rawAmountToHeal);

  if (outcome == CRIT) rawAmountToHeal *= 2;

  auto amountToHeal = chooseRandomSpellMagnitude(rawAmountToHeal);
  target.healBy(amountToHeal);

  return outcome;
}

CombatResult SpellEffect::scaleThreat(const SpellEffect &effect, Entity &caster,
                                      Entity &target,
                                      const std::string &supplementaryArg) {
  auto outcome = caster.generateHitAgainst(target, THREAT_MOD, effect._school,
                                           effect._range);
  if (outcome == MISS) return outcome;

  auto multiplier = effect._args.d1;

  if (outcome == CRIT) multiplier = multiplier * multiplier;

  target.scaleThreatAgainst(caster, multiplier);

  return outcome;
}

CombatResult SpellEffect::buff(const SpellEffect &effect, Entity &caster,
                               Entity &target,
                               const std::string &supplementaryArg) {
  const auto &server = Server::instance();
  const auto *buffType = server.getBuffByName(effect._args.s1);
  if (buffType == nullptr) return FAIL;

  target.applyBuff(*buffType, caster);

  return HIT;
}

CombatResult SpellEffect::debuff(const SpellEffect &effect, Entity &caster,
                                 Entity &target,
                                 const std::string &supplementaryArg) {
  const auto &server = Server::instance();
  const auto *debuffType = server.getBuffByName(effect._args.s1);
  if (debuffType == nullptr) return FAIL;

  auto outcome =
      caster.generateHitAgainst(target, DEBUFF, effect._school, effect._range);

  target.applyDebuff(*debuffType, caster);
  target.onAttackedBy(caster, 0, outcome);

  return HIT;
}

CombatResult SpellEffect::dispellDebuff(const SpellEffect &effect,
                                        Entity &caster, Entity &target,
                                        const std::string &supplementaryArg) {
  const auto &server = Server::instance();
  auto schoolToDispell = SpellSchool{effect._args.s1};
  auto numDebuffsInThatSchool = 0;
  for (const auto &debuff : target.debuffs()) {
    if (debuff.school() == schoolToDispell) ++numDebuffsInThatSchool;
  }
  if (numDebuffsInThatSchool == 0) return HIT;  // Success, but no effect.
  auto debuffToDispell = rand() % numDebuffsInThatSchool;
  auto i = 0;
  for (const auto &debuff : target.debuffs()) {
    if (debuff.school() == schoolToDispell) {
      if (i == debuffToDispell) {
        target.removeDebuff(debuff.type());
        break;
      }
      ++i;
    }
  }

  return HIT;
}

CombatResult SpellEffect::randomTeleport(const SpellEffect &effect,
                                         Entity &caster, Entity &target,
                                         const std::string &supplementaryArg) {
  auto &server = Server::instance();

  auto proposedLocation = MapPoint{};

  auto attempts = 50;
  while (--attempts > 0) {
    auto angle = randDouble() * 2 * PI;
    auto radius = Podes{effect._args.i1}.toPixels();
    auto dX = cos(angle) * radius;
    auto dY = sin(angle) * radius;

    proposedLocation = target.location() + MapPoint{dX, dY};
    if (server.isLocationValid(proposedLocation, target)) break;
  }

  if (attempts == 0) return FAIL;

  target.teleportTo(proposedLocation);

  return HIT;
}

CombatResult SpellEffect::teleportToCity(const SpellEffect &effect,
                                         Entity &caster, Entity &target,
                                         const std::string &supplementaryArg) {
  auto &server = Server::instance();
  const auto *casterAsUser = dynamic_cast<const User *>(&caster);
  if (!casterAsUser) return FAIL;

  auto cityName = server.cities().getPlayerCity(casterAsUser->name());
  if (cityName.empty()) {
    casterAsUser->sendMessage(ERROR_NOT_IN_CITY);
    return FAIL;
  }

  auto cityLoc = server.cities().locationOf(cityName);

  const auto succeeded = target.teleportToValidLocationInCircle(cityLoc, 150.0);
  return succeeded ? HIT : FAIL;
}

CombatResult SpellEffect::teleportToHouse(const SpellEffect &effect,
                                          Entity &caster, Entity &target,
                                          const std::string &supplementaryArg) {
  auto &server = Server::instance();
  const auto *casterAsUser = dynamic_cast<const User *>(&caster);
  if (!casterAsUser) return FAIL;

  const auto userOwnsAHouse = casterAsUser->hasPlayerUnique("house");
  if (!userOwnsAHouse) {
    casterAsUser->sendMessage(WARNING_NO_HOUSE);
    return FAIL;
  }

  const auto *house = server.findUsersHouse(*casterAsUser);
  if (!house) {
    casterAsUser->sendMessage(WARNING_NO_HOUSE);
    return FAIL;
  }

  const auto succeeded =
      target.teleportToValidLocationInCircle(house->location(), 150.0);
  return succeeded ? HIT : FAIL;
}

CombatResult SpellEffect::teachRecipe(const SpellEffect &effect, Entity &caster,
                                      Entity &target,
                                      const std::string &supplementaryArg) {
  auto *casterAsUser = dynamic_cast<User *>(&caster);
  const auto &recipeID = supplementaryArg;

  if (casterAsUser->knowsRecipe(recipeID)) return FAIL;

  casterAsUser->addRecipe(recipeID, true);
  casterAsUser->sendMessage({SV_NEW_RECIPES_LEARNED, makeArgs("1", recipeID)});
  return HIT;
}

CombatResult SpellEffect::spawnNPC(const SpellEffect &effect, Entity &caster,
                                   Entity &target,
                                   const std::string &supplementaryArg) {
  auto &server = Server::instance();

  const auto *objType = server.findObjectTypeByID(effect._args.s1);
  const auto *npcType = dynamic_cast<const NPCType *>(objType);
  if (!npcType) return FAIL;

  auto proposedLocation = MapPoint{};
  const auto MAX_RADIUS = 100_px;

  auto attempts = 100;
  while (attempts-- > 0) {
    auto angle = randDouble() * 2 * PI;
    auto radius = sqrt(randDouble()) * MAX_RADIUS;
    auto dX = cos(angle) * radius;
    auto dY = sin(angle) * radius;

    proposedLocation = caster.location() + MapPoint{dX, dY};
    if (server.isLocationValid(proposedLocation, *npcType)) break;
  }

  if (attempts == 0) return FAIL;

  server.addNPC(npcType, proposedLocation);

  return HIT;
}

CombatResult SpellEffect::randomBuff(const SpellEffect &effect, Entity &caster,
                                     Entity &target,
                                     const std::string &supplementaryArg) {
  auto &server = Server::instance();

  auto buffChoices = std::vector<std::string>{};

  auto numBuffChoices = effect._args.i1;
  if (numBuffChoices == 0) return FAIL;
  if (numBuffChoices >= 1) buffChoices.push_back(effect._args.s1);
  if (numBuffChoices >= 2) buffChoices.push_back(effect._args.s2);
  if (numBuffChoices >= 3) buffChoices.push_back(effect._args.s3);

  auto choiceIndex = rand() % numBuffChoices;
  auto chosenBuff = buffChoices[choiceIndex];

  const auto *buff = server.findBuff(chosenBuff);
  if (!buff) return FAIL;
  target.applyBuff(*buff, caster);

  return HIT;
}

CombatResult SpellEffect::playTwoUp(const SpellEffect &effect, Entity &caster,
                                    Entity &target,
                                    const std::string &supplementaryArg) {
  const auto *casterAsUser = dynamic_cast<const User *>(&caster);
  if (!casterAsUser) return FAIL;
  auto &server = Server::instance();

  const auto possibleResults = std::vector<MessageCode>{
      SV_TWO_UP_WIN, SV_TWO_UP_LOSE, SV_TWO_UP_DRAW, SV_TWO_UP_DRAW};
  const auto randomIndex = rand() % possibleResults.size();
  const auto gameResult = possibleResults[randomIndex];

  server.broadcastToArea(caster.location(), {gameResult, casterAsUser->name()});

  return HIT;
}

Hitpoints SpellEffect::chooseRandomSpellMagnitude(double raw) {
  const auto STANDARD_DEVIATION_MULTIPLIER = double{0.1};
  auto sd = raw * STANDARD_DEVIATION_MULTIPLIER;
  auto bellCurve = NormalVariable{raw, sd};
  auto randomMagnitude = bellCurve.generate();
  return toInt(randomMagnitude);
}
