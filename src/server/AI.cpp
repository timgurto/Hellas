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
    case PET_FOLLOW_OWNER:
      calculatePath();
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
      if (targetHasMoved()) calculatePath();
      if (_owner.location() == _path.currentWaypoint())
        _path.changeToNextWaypoint();
      if (!_path.exists()) break;

      const auto result = _owner.moveLegallyTowards(_path.currentWaypoint());
      if (result == Entity::DID_NOT_MOVE) {
        const auto destination = state == CHASE
                                     ? _owner.target()->location()
                                     : _owner.followTarget()->location();
        _path.findPathToLocation(destination);
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

void AI::Path::findPathToLocation(const MapPoint &destination) {
  // A* on a 25x25 grid
  const auto GRID = 25.0;
  const auto DIAG = sqrt(GRID * GRID + GRID * GRID);
  const auto CLOSE_ENOUGH = DIAG;
  const auto FOOTPRINT = _owner.type()->collisionRect();

  struct UniqueMapPointOrdering {
    bool operator()(const MapPoint &lhs, const MapPoint &rhs) const {
      if (lhs.x != rhs.x) return lhs.x < rhs.x;
      return lhs.y < rhs.y;
    }
  };
  struct AStarNode {
    double g{0};  // Length of the best path to get here
    double f{0};  // g + heuristic (cartesian distance from destination)
    MapPoint parentInBestPath;
  };
  std::map<MapPoint, AStarNode, UniqueMapPointOrdering> nodesByPoint;

  class PointsByFCost {
   public:
    void add(double fCost, MapPoint point) {
      _container.insert(std::make_pair(fCost, point));
    }
    void remove(double fCost, MapPoint point) {
      auto lo = _container.lower_bound(fCost);
      auto hi = _container.upper_bound(fCost);
      for (auto it = lo; it != hi; ++it) {
        if (it->second == point) {
          _container.erase(it);
          return;
        }
      }
    }
    bool stillSomeLeft() const { return !_container.empty(); }
    MapPoint getBestCandidate() const { return _container.begin()->second; };
    void removeBest() { _container.erase(_container.begin()); }

   private:
    std::multimap<double, MapPoint> _container;
  } candidatePoints;

  auto tracePathTo = [&](MapPoint endpoint) {
    auto pathInReverse = std::vector<MapPoint>{};
    auto point = endpoint;
    while (point != _owner.location()) {
      pathInReverse.push_back(point);
      point = nodesByPoint[point].parentInBestPath;
    }
    // Reverse
    auto path = std::queue<MapPoint>{};
    for (auto rIt = pathInReverse.rbegin(); rIt != pathInReverse.rend(); ++rIt)
      path.push(*rIt);
    return path;
  };

  // Start with the current location as the first node
  auto startNode = AStarNode{};
  const auto startPoint = _owner.location();
  startNode.f = distance(startPoint, destination);
  nodesByPoint[startPoint] = startNode;
  candidatePoints.add(startNode.f, startPoint);

  while (candidatePoints.stillSomeLeft()) {
    // Work from the point with the best F cost
    const auto bestCandidatePoint = candidatePoints.getBestCandidate();
    candidatePoints.removeBest();

    if (distance(bestCandidatePoint, destination) <= CLOSE_ENOUGH) {
      _queue = tracePathTo(bestCandidatePoint);
      return;
    }

    // Try going straight to the destination from here
    const auto deltaToDestination = destination - bestCandidatePoint;
    auto journeyRectToDestination = FOOTPRINT + bestCandidatePoint;
    if (deltaToDestination.x < 0)
      journeyRectToDestination.x += deltaToDestination.x;
    if (deltaToDestination.y < 0)
      journeyRectToDestination.y += deltaToDestination.y;
    journeyRectToDestination.w += abs(deltaToDestination.x);
    journeyRectToDestination.h += abs(deltaToDestination.y);
    if (Server::instance().isLocationValid(journeyRectToDestination, _owner)) {
      _queue = tracePathTo(bestCandidatePoint);
      _queue.push(destination);
      return;
    }

    // Calculate F, set to this if existing entry is higher or missing
    auto considerExtension = [&](MapPoint delta, MapRect deltaForJourneyRect,
                                 double extraGCost) {
      const auto nextPoint = bestCandidatePoint + delta;
      auto stepRect = FOOTPRINT + bestCandidatePoint + deltaForJourneyRect;
      if (Server::instance().isLocationValid(stepRect, _owner)) {
        const auto currentNode = nodesByPoint[bestCandidatePoint];
        auto nextNode = AStarNode{};
        nextNode.parentInBestPath = bestCandidatePoint;
        nextNode.g = currentNode.g + extraGCost;
        const auto h = distance(nextPoint, destination);
        nextNode.f = nextNode.g + h;
        auto nodeIter = nodesByPoint.find(nextPoint);
        const auto pointHasntBeenVisited = nodeIter == nodesByPoint.end();
        const auto pointIsBetterFromThisPath =
            !pointHasntBeenVisited && nodeIter->second.f > nextNode.f;
        const auto shouldInsertThisNode =
            pointHasntBeenVisited || pointIsBetterFromThisPath;
        if (pointIsBetterFromThisPath)
          candidatePoints.remove(nodeIter->second.f, nextPoint);
        if (shouldInsertThisNode) {
          candidatePoints.add(nextNode.f, nextPoint);
          nodesByPoint[nextPoint] = nextNode;
        }
      }
    };

    // clang-format off
    considerExtension({ +GRID, -GRID }, {     0, -GRID, +GRID, +GRID }, DIAG); // E
    considerExtension({ +GRID, +GRID }, {     0,     0, +GRID, +GRID }, DIAG); // F
    considerExtension({ -GRID, +GRID }, { -GRID,     0, +GRID, +GRID }, DIAG); // G
    considerExtension({ -GRID, -GRID }, { -GRID, -GRID, +GRID, +GRID }, DIAG); // H
    considerExtension({     0, -GRID }, {     0, -GRID,     0, +GRID }, GRID); // Up
    considerExtension({     0, +GRID }, {     0,     0,     0, +GRID }, GRID); // Down
    considerExtension({ -GRID,     0 }, { -GRID,     0, +GRID,     0 }, GRID); // Left
    considerExtension({ +GRID,     0 }, {     0,     0, +GRID,     0 }, GRID); // Right
    // clang-format on
  }

  clear();
}

void AI::calculatePath() { _path.findPathToLocation(getTargetLocation()); }

bool AI::targetHasMoved() const {
  if (!_path.exists()) return false;

  const auto expectedLocation = _path.lastWaypoint();
  const auto currentLocation = getTargetLocation();

  const auto MAX_TARGET_MOVEMENT_BEFORE_REPATH_SQUARED = 900.0;
  return distanceSquared(expectedLocation, currentLocation) >
         MAX_TARGET_MOVEMENT_BEFORE_REPATH_SQUARED;
}

MapPoint AI::getTargetLocation() const {
  if (state == AI::CHASE) return _owner.target()->location();
  if (state == AI::PET_FOLLOW_OWNER) return _owner.followTarget()->location();
  return {};
}
