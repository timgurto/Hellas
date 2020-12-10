#include "AI.h"

#include "NPC.h"
#include "Server.h"
#include "User.h"

AI::AI(NPC &owner) : _owner(owner), _path(owner) {
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

  switch (state) {
    case IDLE:
      // There's a target to attack
      {
        auto canAttack =
            _owner.combatDamage() > 0 || _owner.npcType()->knownSpell();
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

      if (!_path.exists()) {
        state = IDLE;
        break;
      }
    }

    case CHASE:
      // Target has disappeared
      if (!_owner.target()) {
        state = IDLE;
        break;
      }

      // Target is dead
      if (_owner.target()->isDead()) {
        state = IDLE;
        break;
      }

      // NPC has gone too far from home location
      {
        auto distFromHome = distance(_homeLocation, _owner.location());
        if (distFromHome > _owner.npcType()->maxDistanceFromHome()) {
          state = IDLE;
          _path.clear();
          _owner.teleportTo(_homeLocation);
          break;
        }
      }

      // Target has run out of range: give up
      if (!_owner.npcType()->pursuesEndlessly() &&
          distToTarget > PURSUIT_RANGE) {
        state = IDLE;
        _owner.tagger.clear();
        break;
      }

      // Target is close enough to attack
      if (distToTarget <= ATTACK_RANGE) {
        state = ATTACK;
        break;
      }

      if (!_path.exists()) {
        state = IDLE;
        break;
      }

      break;

    case AI::ATTACK:
      // Target has disappeared
      if (_owner.target() == nullptr) {
        state = IDLE;
        break;
      }

      // Target is dead
      if (_owner.target()->isDead()) {
        state = IDLE;
        break;
      }

      // Target has run out of attack range: chase
      if (distToTarget > ATTACK_RANGE) {
        state = CHASE;
        _owner.resetLocationUpdateTimer();
        break;
      }

      break;
  }
}

void AI::onTransition(State previousState) {
  auto previousLocation = _owner.location();

  switch (state) {
    case IDLE: {
      if (previousState != CHASE && previousState != ATTACK) break;
      _owner.target(nullptr);
      _owner._threatTable.clear();
      _owner.tagger.clear();

      if (_owner.owner().type != Permissions::Owner::ALL_HAVE_ACCESS) return;

      const auto ATTEMPTS = 20;
      for (auto i = 0; i != ATTEMPTS; ++i) {
        if (!_owner.spawner()) break;

        auto dest = _owner.spawner()->getRandomPoint();
        if (Server::instance().isLocationValid(dest, *_owner.type())) {
          _path.clear();
          _owner.teleportTo(dest);
          break;
        }
      }

      auto maxHealth = _owner.type()->baseStats().maxHealth;
      if (_owner.health() < maxHealth) {
        _owner.health(maxHealth);
        _owner.onHealthChange();  // Only broadcasts to the new location, not
                                  // the old.

        for (const User *user :
             Server::instance().findUsersInArea(previousLocation))
          user->sendMessage(
              {SV_ENTITY_HEALTH, makeArgs(_owner.serial(), _owner.health())});
      }
      break;
    }

    case CHASE:
      _path.findIndirectPathTo(_owner.target()->location());
      break;

    case PET_FOLLOW_OWNER:
      _path.findIndirectPathTo(_owner.followTarget()->location());
      break;
  }
}

void AI::act() {
  switch (state) {
    case IDLE:
      _owner.target(nullptr);
      break;

    case PET_FOLLOW_OWNER:
    case CHASE: {
      // Move towards target
      if (!_path.exists()) break;
      if (_owner.location() == _path.currentWaypoint())
        _path.changeToNextWaypoint();
      const auto result = _owner.moveLegallyTowards(_path.currentWaypoint());
      if (result == Entity::DID_NOT_MOVE) {
        const auto destination = state == CHASE
                                     ? _owner.target()->location()
                                     : _owner.followTarget()->location();
        _path.findIndirectPathTo(destination);
      }
      break;
    }

    case ATTACK:
      // Cast any spells it knows
      auto knownSpell = _owner.npcType()->knownSpell();
      if (knownSpell) {
        if (_owner.isSpellCoolingDown(knownSpell->id())) break;
        _owner.castSpell(*knownSpell);
      }
      break;  // Entity::update() will handle combat
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

void AI::Path::findIndirectPathTo(const MapPoint &destination) {
  // 25x25 breadth-first search
  const auto GRID_SIZE = 25.0;
  const auto CLOSE_ENOUGH = sqrt(GRID_SIZE * GRID_SIZE + GRID_SIZE * GRID_SIZE);

  class PossiblePath {
   public:
    PossiblePath(const NPC &owner)
        : _owner(owner), _footprint(owner.type()->collisionRect()) {}

    void addWaypoint(const MapPoint &nextWaypoint) {
      _waypoints.push(nextWaypoint);
      _lastWaypoint = nextWaypoint;
    }

    const MapPoint &lastWaypoint() const { return _lastWaypoint; }
    const std::queue<MapPoint> &waypoints() const { return _waypoints; }

    PossiblePath extendUp() {
      auto newPath = *this;
      auto betweenWaypoints = _footprint + _lastWaypoint;
      betweenWaypoints.y -= GRID_SIZE;
      betweenWaypoints.h += GRID_SIZE;
      if (Server::instance().isLocationValid(betweenWaypoints, _owner)) {
        auto nextWaypoint = newPath.lastWaypoint() + MapPoint{0, -GRID_SIZE};
        newPath.addWaypoint(nextWaypoint);
      }
      return newPath;
    }

    PossiblePath extendDown() {
      auto newPath = *this;
      auto betweenWaypoints = _footprint + _lastWaypoint;
      betweenWaypoints.h += GRID_SIZE;
      if (Server::instance().isLocationValid(betweenWaypoints, _owner)) {
        auto nextWaypoint = newPath.lastWaypoint() + MapPoint{0, +GRID_SIZE};
        newPath.addWaypoint(nextWaypoint);
      }
      return newPath;
    }

    PossiblePath extendLeft() {
      auto newPath = *this;
      auto betweenWaypoints = _footprint + _lastWaypoint;
      betweenWaypoints.x -= GRID_SIZE;
      betweenWaypoints.w += GRID_SIZE;
      if (Server::instance().isLocationValid(betweenWaypoints, _owner)) {
        auto nextWaypoint = newPath.lastWaypoint() + MapPoint{-GRID_SIZE, 0};
        newPath.addWaypoint(nextWaypoint);
      }
      return newPath;
    }

    PossiblePath extendRight() {
      auto newPath = *this;
      auto betweenWaypoints = _footprint + _lastWaypoint;
      betweenWaypoints.w += GRID_SIZE;
      if (Server::instance().isLocationValid(betweenWaypoints, _owner)) {
        auto nextWaypoint = newPath.lastWaypoint() + MapPoint{+GRID_SIZE, 0};
        newPath.addWaypoint(nextWaypoint);
      }
      return newPath;
    }

    PossiblePath extendTo(const MapPoint &destination) {
      auto newPath = *this;
      auto betweenWaypoints = _footprint + _lastWaypoint;
      auto dX = destination.x - _lastWaypoint.x;
      auto dY = destination.y - _lastWaypoint.y;
      if (dX < 0) betweenWaypoints.x += dX;
      if (dY < 0) betweenWaypoints.y += dY;
      betweenWaypoints.w += abs(dX);
      betweenWaypoints.h += abs(dY);
      if (Server::instance().isLocationValid(betweenWaypoints, _owner)) {
        newPath.addWaypoint(destination);
      }
      return newPath;
    }

   private:
    std::queue<MapPoint> _waypoints;
    MapPoint _lastWaypoint;
    const NPC &_owner;
    MapRect _footprint;
  };

  struct UniqueMapPointOrdering {
    bool operator()(const MapPoint &lhs, const MapPoint &rhs) const {
      if (lhs.x != rhs.x) return lhs.x < rhs.x;
      return lhs.y < rhs.y;
    }
  };
  auto pointsCovered = std::set<MapPoint, UniqueMapPointOrdering>{};

  auto pathsUnderConsideration = std::queue<PossiblePath>{};

  auto starterPath = PossiblePath{_owner};
  starterPath.addWaypoint(_owner.location());
  pathsUnderConsideration.push(starterPath);
  pointsCovered.insert(_owner.location());

  while (!pathsUnderConsideration.empty()) {
    auto currentPath = pathsUnderConsideration.front();
    if (distance(currentPath.lastWaypoint(), destination) <= CLOSE_ENOUGH) {
      _queue = currentPath.waypoints();
      return;
    }

    auto extendedToDestination = currentPath.extendTo(destination);
    if (extendedToDestination.lastWaypoint() == destination &&
        !pointsCovered.count(destination)) {
      _queue = extendedToDestination.waypoints();
      return;
    }

    auto extendedUp = currentPath.extendUp();
    if (!pointsCovered.count(extendedUp.lastWaypoint())) {
      pointsCovered.insert(extendedUp.lastWaypoint());
      pathsUnderConsideration.push(extendedUp);
    }

    auto extendedDown = currentPath.extendDown();
    if (!pointsCovered.count(extendedDown.lastWaypoint())) {
      pointsCovered.insert(extendedDown.lastWaypoint());
      pathsUnderConsideration.push(extendedDown);
    }

    auto extendedLeft = currentPath.extendLeft();
    if (!pointsCovered.count(extendedLeft.lastWaypoint())) {
      pointsCovered.insert(extendedLeft.lastWaypoint());
      pathsUnderConsideration.push(extendedLeft);
    }

    auto extendedRight = currentPath.extendRight();
    if (!pointsCovered.count(extendedRight.lastWaypoint())) {
      pointsCovered.insert(extendedRight.lastWaypoint());
      pathsUnderConsideration.push(extendedRight);
    }

    pathsUnderConsideration.pop();
  }

  clear();
}
