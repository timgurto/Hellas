#include "NPC.h"
#include "Server.h"

void NPC::processAI(ms_t timeElapsed) {
  target(nullptr);

  getNewTargetsFromProximity(timeElapsed);

  target(_threatTable.getTarget());

  State previousState = _state;

  transitionIfNecessary();
  if (_state != previousState) onTransition(previousState);
  act();
}

void NPC::getNewTargetsFromProximity(ms_t timeElapsed) {
  if (!npcType()->attacksNearby()) return;

  _timeSinceLookedForTargets += timeElapsed;
  if (_timeSinceLookedForTargets < FREQUENCY_TO_LOOK_FOR_TARGETS) return;
  _timeSinceLookedForTargets =
      _timeSinceLookedForTargets % FREQUENCY_TO_LOOK_FOR_TARGETS;

  for (auto *potentialTarget :
       Server::_instance->findEntitiesInArea(location(), AGGRO_RANGE)) {
    if (potentialTarget == this) continue;
    if (!potentialTarget->canBeAttackedBy(*this)) continue;
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

  switch (_state) {
    case IDLE:
      // There's a target to attack
      {
        auto canAttack = combatDamage() > 0 || npcType()->knownSpell();
        if (canAttack && target()) {
          if (distToTarget > ATTACK_RANGE) {
            _state = CHASE;
            resetLocationUpdateTimer();
          } else
            _state = ATTACK;
          break;
        }
      }

      // Follow owner, if nearby
      {
        if (owner().type != Permissions::Owner::PLAYER) break;
        const auto *ownerPlayer =
            Server::instance().getUserByName(owner().name);
        if (ownerPlayer == nullptr) break;
        if (distance(ownerPlayer->collisionRect(), collisionRect()) <=
            FOLLOW_DISTANCE)
          break;
        _state = FOLLOW;
        _followTarget = ownerPlayer;
        break;
      }

      break;

    case FOLLOW:
      // There's a target to attack
      if (target()) {
        if (distToTarget > ATTACK_RANGE) {
          _state = CHASE;
          resetLocationUpdateTimer();
        } else
          _state = ATTACK;
        break;
      }

      // Owner is close enough
      if (distance(_followTarget->collisionRect(), collisionRect()) <=
          FOLLOW_DISTANCE) {
        _state = IDLE;
        _followTarget = nullptr;
        break;
      }

    case CHASE:
      // Target has disappeared
      if (!target()) {
        _state = IDLE;
        break;
      }

      // Target is dead
      if (target()->isDead()) {
        _state = IDLE;
        break;
      }

      // NPC has gone too far from spawn point
      if (spawner()) {
        auto distFromSpawner = spawner()->distanceFromEntity(*this);
        if (distFromSpawner > npcType()->maxDistanceFromSpawner()) {
          _state = IDLE;
          static const auto ATTEMPTS = 20;
          for (auto i = 0; i != ATTEMPTS; ++i) {
            auto dest = spawner()->getRandomPoint();
            if (Server::instance().isLocationValid(dest, *type())) {
              _targetDestination = dest;
              teleportTo(_targetDestination);
              break;
            }
          }
          break;
        }
      }

      // Target has run out of range: give up
      if (distToTarget > PURSUIT_RANGE) {
        _state = IDLE;
        clearTagger();
        break;
      }

      // Target is close enough to attack
      if (distToTarget <= ATTACK_RANGE) {
        _state = ATTACK;
        break;
      }

      break;

    case ATTACK:
      // Target has disappeared
      if (target() == nullptr) {
        _state = IDLE;
        break;
      }

      // Target is dead
      if (target()->isDead()) {
        _state = IDLE;
        break;
      }

      // Target has run out of attack range: chase
      if (distToTarget > ATTACK_RANGE) {
        _state = CHASE;
        resetLocationUpdateTimer();
        break;
      }

      break;
  }
}

void NPC::onTransition(State previousState) {
  auto previousLocation = location();

  if ((previousState == CHASE || previousState == ATTACK) && _state == IDLE) {
    target(nullptr);
    _threatTable.clear();
    clearTagger();

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
  switch (_state) {
    case IDLE:
      target(nullptr);
      break;

    case FOLLOW:
      moveLegallyTowards(_followTarget->location());
      break;

    case CHASE:
      // Move towards target
      moveLegallyTowards(target()->location());
      break;

    case ATTACK:
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
