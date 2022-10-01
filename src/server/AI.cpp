#include "AI.h"

#include "NPC.h"
#include "Server.h"
#include "User.h"

AI::AI(NPC &owner) : Pathfinder(owner), _owner(owner) {
  _homeLocation = _owner.location();
}

void AI::process(ms_t timeElapsed) {
  _owner.target(nullptr);

  _owner.getNewTargetsFromProximity(timeElapsed);

  _owner.target(_owner._threatTable.getTarget());

  const auto previousState = state;

  transitionIfNecessary();
  if (state != previousState) onTransition(previousState);
  act();
}

void AI::transitionIfNecessary() {
  const auto ATTACK_RANGE = _owner.attackRange() - Podes{1}.toPixels();

  auto distToTarget = _owner.target() ? distance(_owner, *_owner.target()) : 0;

  auto findNewTargetOrRetreat = [&]() {
    _owner._threatTable.forgetCurrentTarget();
    _owner.target(_owner._threatTable.getTarget());
    if (_owner.target())
      state = CHASE;
    else
      state = RETREAT;
  };

  switch (state) {
    case IDLE:
      // There's a target to attack
      {
        auto canAttack = _owner.combatDamage() > 0 ||
                         _owner.npcType()->knownSpells().size() >= 1;
        if (canAttack && _owner.target()) {
          if (distToTarget > ATTACK_RANGE) {
            state = CHASE;
            _owner.resetLocationUpdateTimer();
          } else
            state = ATTACK;
          break;
        }
      }

      // Follow owner, if nearby
      {
        if (order != ORDER_TO_FOLLOW) break;
        if (_owner.owner().type != Permissions::Owner::PLAYER) break;
        const auto *ownerPlayer =
            Server::instance().getUserByName(_owner.owner().name);
        if (ownerPlayer == nullptr) break;
        if (distance(*ownerPlayer, _owner) <= FOLLOW_DISTANCE) break;
        state = PET_FOLLOW_OWNER;
        _owner._followTarget = ownerPlayer;
        break;
      }

      break;

    case PET_FOLLOW_OWNER: {
      if (order == ORDER_TO_STAY) {
        state = IDLE;
        break;
      }

      // There's a target to attack
      if (_owner.target()) {
        if (distToTarget > ATTACK_RANGE) {
          state = CHASE;
          _owner.resetLocationUpdateTimer();
        } else
          state = ATTACK;
        break;
      }

      const auto distanceFromOwner = distance(*_owner._followTarget, _owner);

      // Owner is close enough
      if (distanceFromOwner <= AI::FOLLOW_DISTANCE) {
        state = IDLE;
        _owner._followTarget = nullptr;
        break;
      }

      // Owner is too far away
      if (distanceFromOwner >= MAX_FOLLOW_RANGE) {
        state = IDLE;
        order = ORDER_TO_STAY;

        if (_owner.owner().type == Permissions::Owner::PLAYER) {
          auto *ownerPlayer =
              Server::instance().getUserByName(_owner.owner().name);
          if (ownerPlayer) ownerPlayer->followers.remove();
        }

        _owner._followTarget = nullptr;
        break;
      }
      break;

      if (!_activePath.exists()) {
        state = IDLE;
        break;
      }
    }

    case CHASE:
      // Target's gone
      if (!_owner.target() || _owner.target()->isDead()) {
        findNewTargetOrRetreat();
        break;
      }

      // NPC has gone too far from home location
      {
        auto distFromHome = distance(_homeLocation, _owner.location());
        if (distFromHome > _owner.npcType()->maxDistanceFromHome()) {
          state = RETREAT;
          break;
        }
      }

      // Target has run out of range: give up
      if (!_owner.npcType()->pursuesEndlessly() &&
          distToTarget > PURSUIT_RANGE) {
        findNewTargetOrRetreat();
        break;
      }

      // Target is close enough to attack
      if (distToTarget <= ATTACK_RANGE) {
        state = ATTACK;
        break;
      }

      // Previous pathfinding attempt failed
      if (_failedToFindPath) {
        _failedToFindPath = false;
        state = RETREAT;
        break;
      }

      break;

    case AI::ATTACK:
      // Target's gone
      if (!_owner.target() || _owner.target()->isDead()) {
        findNewTargetOrRetreat();
        break;
      }

      // Target has run out of attack range: chase
      if (distToTarget > ATTACK_RANGE) {
        state = CHASE;
        _owner.resetLocationUpdateTimer();
        break;
      }

      break;

    case AI::RETREAT:
      // Reached target
      const auto CLOSE_ENOUGH = Server::ACTION_DISTANCE;
      if (distance(_owner.location(), _homeLocation) <= CLOSE_ENOUGH) {
        state = IDLE;
        break;
      }

      // Previous pathfinding attempt failed: pick new location and try again
      if (_failedToFindPath) {
        _failedToFindPath = false;
        pickRandomSpotNearSpawnPoint();
        break;
      }
  }
}

void AI::onTransition(State previousState) {
  auto previousLocation = _owner.location();

  switch (state) {
    case CHASE:
    case PET_FOLLOW_OWNER:
      calculatePathInSeparateThread();
      break;

    case RETREAT: {
      _owner.target(nullptr);
      _owner._threatTable.clear();
      _owner.tagger.clear();

      const auto isPetAndShouldFollow = _owner.permissions.hasOwner();
      if (isPetAndShouldFollow)
        break;
      else {
        pickRandomSpotNearSpawnPoint();
        _owner.restoreHealthAndBroadcastTo(previousLocation);
      }
      break;
    }
  }
}

void AI::act() {
  switch (state) {
    case IDLE:
      _owner.target(nullptr);
      break;

    case PET_FOLLOW_OWNER:
    case CHASE:
    case RETREAT: {
      if (!_activePath.exists()) {
        calculatePathInSeparateThread();
        break;
      }

      // Move towards target
      if (targetHasMoved()) {
        calculatePathInSeparateThread();
        break;
      }

      // If reached the current waypoint, move to the next one
      if (_owner.location() == _activePath.currentWaypoint())
        _activePath.changeToNextWaypoint();
      if (!_activePath.exists()) break;

      const auto result =
          _owner.moveLegallyTowards(_activePath.currentWaypoint());
      if (result == Entity::MOVED_INTO_OBSTACLE)
        calculatePathInSeparateThread();

      break;
    }

    case ATTACK:
      // Cast any spells it knows
      for (const auto &knownSpell : _owner.npcType()->knownSpells()) {
        if (!knownSpell.spell) continue;
        if (_owner.isSpellCoolingDown(knownSpell.id)) continue;
        _owner.castSpell(*knownSpell.spell);
      }

      break;  // Entity::update() will handle combat
  }

  // Cast any allowed-out-of-combat spells it knows
  for (const auto &knownSpell : _owner.npcType()->knownSpells()) {
    if (!knownSpell.spell) continue;
    if (!knownSpell.canCastOutOfCombat) continue;
    if (_owner.isSpellCoolingDown(knownSpell.id)) continue;
    _owner.castSpell(*knownSpell.spell);
  }
}

void AI::giveOrder(PetOrder newOrder) {
  order = newOrder;
  _homeLocation = _owner.location();

  // Send order confirmation to owner
  auto owner = _owner.permissions.getPlayerOwner();
  if (!owner) return;
  auto serialArg = makeArgs(_owner.serial());
  switch (order) {
    case ORDER_TO_STAY:
      owner->sendMessage({SV_PET_IS_NOW_STAYING, serialArg});
      break;

    case ORDER_TO_FOLLOW:
      owner->sendMessage({SV_PET_IS_NOW_FOLLOWING, serialArg});
      break;

    default:
      break;
  }
}

bool AI::targetHasMoved() const {
  if (!_activePath.exists()) return false;

  const auto pathEnd = MapRect{_activePath.lastWaypoint()};
  const auto targetLocation = getTargetFootprint();

  const auto MAX_TARGET_MOVEMENT_BEFORE_REPATH = 30.0;
  const auto maxDistanceAllowed =
      MAX_TARGET_MOVEMENT_BEFORE_REPATH + howCloseShouldPathfindingGet();
  const auto gapBetweenTargetAndPathEnd = distance(pathEnd, targetLocation);
  return gapBetweenTargetAndPathEnd > maxDistanceAllowed;
}

MapRect AI::getTargetFootprint() const {
  if (state == AI::CHASE && _owner.target())
    return _owner.target()->collisionRect();
  if (state == AI::PET_FOLLOW_OWNER && _owner.followTarget())
    return _owner.followTarget()->collisionRect();
  if (state == AI::RETREAT)
    return _homeLocation + _owner.type()->collisionRect();

  SERVER_ERROR("Pathfinding while AI is in an inappropriate state");
  return {};
}

double AI::howCloseShouldPathfindingGet() const {
  if (state == AI::CHASE) return _owner.attackRange();
  if (state == AI::PET_FOLLOW_OWNER) return AI::FOLLOW_DISTANCE;
  if (state == AI::RETREAT) return 0;

  SERVER_ERROR("Pathfinding while AI is in an inappropriate state");
  return 0;
}

void AI::pickRandomSpotNearSpawnPoint() {
  if (!_owner.spawner()) return;

  const auto ATTEMPTS = 50;
  for (auto i = 0; i != ATTEMPTS; ++i) {
    auto dest = _owner.spawner()->getRandomPoint();
    if (Server::instance().isLocationValid(dest, _owner)) {
      _activePath.clear();
      _homeLocation = dest;
      return;
    }
  }
}
