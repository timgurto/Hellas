#include "Spell.h"

#include <string>

#include "Server.h"
#include "User.h"

CombatResult Spell::performAction(Entity &caster, Entity &target,
                                  const std::string &supplementaryArg) const {
  if (!_effect.exists()) return FAIL;

  const Server &server = Server::instance();
  const auto *casterAsUser = dynamic_cast<const User *>(&caster);

  auto skipWarnings =
      !casterAsUser ||  // No point warning an NPC
      _effect.isAoE();  // Since there may be many targets, not all valid.

  if (!isTargetValid(caster, target)) {
    if (!skipWarnings) casterAsUser->sendMessage(WARNING_INVALID_SPELL_TARGET);
    return FAIL;
  }

  if (target.isDead()) {
    if (!skipWarnings) casterAsUser->sendMessage(ERROR_TARGET_DEAD);
    return FAIL;
  }

  if (distance(caster, target) > _range) {
    if (!skipWarnings) casterAsUser->sendMessage(WARNING_TOO_FAR);
    return FAIL;
  }

  return _effect.execute(caster, target, supplementaryArg);
}

bool Spell::isTargetValid(const Entity &caster, const Entity &target) const {
  if (&caster == &target) return _validTargets[SELF];

  const auto *casterAsUser = dynamic_cast<const User *>(&caster);
  auto casterIsUser = !!casterAsUser;
  if (casterIsUser) {
    if (target.canBeAttackedBy(*casterAsUser)) return _validTargets[ENEMY];
  } else {
    if (target.classTag() == 'u')
      return _validTargets[ENEMY];  // Assumption: NPCs can attack only Users.
                                    // Assumption: NPCs always treat Users as
                                    // enemies for the purposes of spells.  This
                                    // will need to be changed if, e.g., a
                                    // friendly NPC casts a positive spell on a
                                    // User.
  }

  return _validTargets[FRIENDLY];
}

bool Spell::canCastOnlyOnSelf() const {
  for (auto i = 0; i != _validTargets.size(); ++i)
    if (i != SELF && _validTargets[i]) return false;
  return true;
}
