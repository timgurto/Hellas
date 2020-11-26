#include "NPC.h"
#include "Server.h"

void NPC::order(Order newOrder) {
  _order = newOrder;
  _homeLocation = location();

  // Send order confirmation to owner
  auto owner = permissions.getPlayerOwner();
  if (!owner) return;
  auto serialArg = makeArgs(serial());
  switch (_order) {
    case NPC::STAY:
      owner->sendMessage({SV_PET_IS_NOW_STAYING, serialArg});
      break;

    case NPC::FOLLOW:
      owner->sendMessage({SV_PET_IS_NOW_FOLLOWING, serialArg});
      break;

    default:
      break;
  }
}

void NPC::processAI(ms_t timeElapsed) {
  target(nullptr);

  getNewTargetsFromProximity(timeElapsed);

  target(_threatTable.getTarget());

  AI::State previousState = ai.state;

  transitionIfNecessary();
  if (ai.state != previousState) onTransition(previousState);
  act();
}

void NPC::getNewTargetsFromProximity(ms_t timeElapsed) {
  auto shouldLookForNewTargetsNearby =
      npcType()->attacksNearby() || permissions.hasOwner();
  if (!shouldLookForNewTargetsNearby) return;

  _timeSinceLookedForTargets += timeElapsed;
  if (_timeSinceLookedForTargets < FREQUENCY_TO_LOOK_FOR_TARGETS) return;
  _timeSinceLookedForTargets =
      _timeSinceLookedForTargets % FREQUENCY_TO_LOOK_FOR_TARGETS;

  for (auto *potentialTarget :
       Server::_instance->findEntitiesInArea(location(), AGGRO_RANGE)) {
    if (potentialTarget == this) continue;
    if (!potentialTarget->canBeAttackedBy(*this)) continue;
    if (potentialTarget->shouldBeIgnoredByAIProximityAggro()) continue;
    if (distance(collisionRect(), potentialTarget->collisionRect()) >
        AGGRO_RANGE)
      continue;

    makeAwareOf(*potentialTarget);
  }
}

void NPC::transitionIfNecessary() {
  const auto ATTACK_RANGE = attackRange() - Podes{1}.toPixels();

  auto distToTarget =
      target() ? distance(collisionRect(), target()->collisionRect()) : 0;

  switch (ai.state) {
    case AI::IDLE:
      // There's a target to attack
      {
        auto canAttack = combatDamage() > 0 || npcType()->knownSpell();
        if (canAttack && target()) {
          if (distToTarget > ATTACK_RANGE) {
            ai.state = AI::CHASE;
            resetLocationUpdateTimer();
          } else
            ai.state = AI::ATTACK;
          break;
        }
      }

      // Follow owner, if nearby
      {
        if (_order != FOLLOW) break;
        if (owner().type != Permissions::Owner::PLAYER) break;
        const auto *ownerPlayer =
            Server::instance().getUserByName(owner().name);
        if (ownerPlayer == nullptr) break;
        if (distance(ownerPlayer->collisionRect(), collisionRect()) <=
            FOLLOW_DISTANCE)
          break;
        ai.state = AI::PET_FOLLOW_OWNER;
        _followTarget = ownerPlayer;
        break;
      }

      break;

    case AI::PET_FOLLOW_OWNER: {
      if (_order == STAY) {
        ai.state = AI::IDLE;
        break;
      }

      // There's a target to attack
      if (target()) {
        if (distToTarget > ATTACK_RANGE) {
          ai.state = AI::CHASE;
          resetLocationUpdateTimer();
        } else
          ai.state = AI::ATTACK;
        break;
      }

      const auto distanceFromOwner =
          distance(_followTarget->collisionRect(), collisionRect());

      // Owner is close enough
      if (distanceFromOwner <= FOLLOW_DISTANCE) {
        ai.state = AI::IDLE;
        _followTarget = nullptr;
        break;
      }

      // Owner is too far away
      if (distanceFromOwner >= MAX_FOLLOW_RANGE) {
        ai.state = AI::IDLE;
        _order = STAY;

        if (owner().type == Permissions::Owner::PLAYER) {
          auto *ownerPlayer = Server::instance().getUserByName(owner().name);
          if (ownerPlayer) ownerPlayer->followers.remove();
        }

        _followTarget = nullptr;
        break;
      }
      break;
    }

    case AI::CHASE:
      // Target has disappeared
      if (!target()) {
        ai.state = AI::IDLE;
        break;
      }

      // Target is dead
      if (target()->isDead()) {
        ai.state = AI::IDLE;
        break;
      }

      // NPC has gone too far from home location
      {
        auto distFromHome = distance(_homeLocation, location());
        if (distFromHome > npcType()->maxDistanceFromHome()) {
          ai.state = AI::IDLE;
          _targetDestination = _homeLocation;
          teleportTo(_targetDestination);
          break;
        }
      }

      // Target has run out of range: give up
      if (distToTarget > PURSUIT_RANGE) {
        ai.state = AI::IDLE;
        tagger.clear();
        break;
      }

      // Target is close enough to attack
      if (distToTarget <= ATTACK_RANGE) {
        ai.state = AI::ATTACK;
        break;
      }

      break;

    case AI::ATTACK:
      // Target has disappeared
      if (target() == nullptr) {
        ai.state = AI::IDLE;
        break;
      }

      // Target is dead
      if (target()->isDead()) {
        ai.state = AI::IDLE;
        break;
      }

      // Target has run out of attack range: chase
      if (distToTarget > ATTACK_RANGE) {
        ai.state = AI::CHASE;
        resetLocationUpdateTimer();
        break;
      }

      break;
  }
}

void NPC::onTransition(AI::State previousState) {
  auto previousLocation = location();

  if ((previousState == AI::CHASE || previousState == AI::ATTACK) &&
      ai.state == AI::IDLE) {
    target(nullptr);
    _threatTable.clear();
    tagger.clear();

    if (owner().type != Permissions::Owner::ALL_HAVE_ACCESS) return;

    static const auto ATTEMPTS = 20;
    for (auto i = 0; i != ATTEMPTS; ++i) {
      if (!spawner()) break;

      auto dest = spawner()->getRandomPoint();
      if (Server::instance().isLocationValid(dest, *type())) {
        _targetDestination = dest;
        teleportTo(_targetDestination);
        break;
      }
    }

    auto maxHealth = type()->baseStats().maxHealth;
    if (health() < maxHealth) {
      health(maxHealth);
      onHealthChange();  // Only broadcasts to the new location, not the old.

      const Server &server = *Server::_instance;
      for (const User *user : server.findUsersInArea(previousLocation))
        user->sendMessage({SV_ENTITY_HEALTH, makeArgs(serial(), health())});
    }
  }
}

void NPC::act() {
  switch (ai.state) {
    case AI::IDLE:
      target(nullptr);
      break;

    case AI::PET_FOLLOW_OWNER:
      moveLegallyTowards(_followTarget->location());
      break;

    case AI::CHASE:
      // Move towards target
      moveLegallyTowards(target()->location());
      break;

    case AI::ATTACK:
      // Cast any spells it knows
      auto knownSpell = npcType()->knownSpell();
      if (knownSpell) {
        if (isSpellCoolingDown(knownSpell->id())) break;
        ;
        castSpell(*knownSpell);
      }
      break;  // Entity::update() will handle combat
  }
}

void NPC::setStateBasedOnOrder() {
  if (_order == STAY)
    ai.state = AI::IDLE;
  else
    ai.state = AI::PET_FOLLOW_OWNER;
}
