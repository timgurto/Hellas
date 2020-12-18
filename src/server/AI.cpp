#include "AI.h"

#include "../threadNaming.h"
#include "NPC.h"
#include "Server.h"
#include "User.h"

AI::AI(NPC &owner) : _owner(owner), _activePath(owner) {
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

      if (!_activePath.exists()) {
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
          _activePath.clear();
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
          _activePath.clear();
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
      calculatePathInSeparateThread();
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
      if (targetHasMoved()) calculatePathInSeparateThread();
      if (!_activePath.exists()) break;

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

void AI::Path::findPathTo(const MapRect &targetFootprint) {
  // A*
  const auto GRID = 25.0;
  const auto DIAG = sqrt(GRID * GRID + GRID * GRID);
  const auto closeEnough = _owner.ai.howCloseShouldPathfindingGet();
  const auto footprint = _owner.type()->collisionRect();

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

  const auto NO_PARENT = MapPoint{-12345, -12345};

  auto tracePathTo = [&](MapPoint endpoint) {
    auto pathInReverse = std::vector<MapPoint>{};
    auto point = endpoint;
    while (point != NO_PARENT) {
      auto it = nodesByPoint.find(point);
      if (it == nodesByPoint.end()) {
        SERVER_ERROR("Tracing path failed; parent not found.");
        break;
      }
      pathInReverse.push_back(point);
      point = it->second.parentInBestPath;
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
  startNode.f = distance(footprint + startPoint, targetFootprint);
  startNode.parentInBestPath = NO_PARENT;
  nodesByPoint[startPoint] = startNode;
  candidatePoints.add(startNode.f, startPoint);

  struct Extension {
    MapPoint delta;
    double distance;
    MapRect journeyRectDelta() const {
      auto ret = MapRect{0, 0, abs(delta.x), abs(delta.y)};
      if (delta.x < 0) ret.x = delta.x;
      if (delta.y < 0) ret.y = delta.y;
      return ret;
    }
  };
  auto extensionCandidates = std::vector<Extension>{};
  extensionCandidates.push_back({{+GRID, -GRID}, DIAG});
  extensionCandidates.push_back({{+GRID, +GRID}, DIAG});
  extensionCandidates.push_back({{-GRID, +GRID}, DIAG});
  extensionCandidates.push_back({{-GRID, -GRID}, DIAG});
  extensionCandidates.push_back({{0, -GRID}, GRID});
  extensionCandidates.push_back({{0, +GRID}, GRID});
  extensionCandidates.push_back({{-GRID, 0}, GRID});
  extensionCandidates.push_back({{+GRID, 0}, GRID});

  while (candidatePoints.stillSomeLeft()) {
    // Work from the point with the best F cost
    const auto bestCandidatePoint = candidatePoints.getBestCandidate();
    candidatePoints.removeBest();

    if (distance(footprint + bestCandidatePoint, targetFootprint) <=
        closeEnough) {
      _queue = tracePathTo(bestCandidatePoint);
      return;
    }

    // Try going straight to the destination from here
    const auto deltaToDestination =
        MapPoint{targetFootprint} - bestCandidatePoint;
    auto journeyRectToDestination = footprint + bestCandidatePoint;
    if (deltaToDestination.x < 0)
      journeyRectToDestination.x += deltaToDestination.x;
    if (deltaToDestination.y < 0)
      journeyRectToDestination.y += deltaToDestination.y;
    journeyRectToDestination.w += abs(deltaToDestination.x);
    journeyRectToDestination.h += abs(deltaToDestination.y);
    if (Server::instance().isLocationValid(journeyRectToDestination, _owner)) {
      _queue = tracePathTo(bestCandidatePoint);
      _queue.push(targetFootprint);
      return;
    }

    // Calculate F, set to this if existing entry is higher or missing
    for (const auto &extension : extensionCandidates) {
      const auto nextPoint = bestCandidatePoint + extension.delta;
      auto stepRect =
          footprint + bestCandidatePoint + extension.journeyRectDelta();
      if (Server::instance().isLocationValid(stepRect, _owner)) {
        const auto currentNode = nodesByPoint[bestCandidatePoint];
        auto nextNode = AStarNode{};
        nextNode.parentInBestPath = bestCandidatePoint;
        nextNode.g = currentNode.g + extension.distance;
        const auto h = distance(footprint + nextPoint, targetFootprint);

        const auto pathStraysTooFar =
            h > PURSUIT_RANGE && !_owner.npcType()->pursuesEndlessly();
        if (pathStraysTooFar) continue;

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
    }
  }

  // If execution gets here, no valid path was found.
  clear();
}

void AI::calculatePathInSeparateThread() {
  const auto targetFootprint = getTargetFootprint();

  std::thread([this, targetFootprint]() {
    setThreadName("Pathfinding for " + _owner.type()->id() + " serial " +
                  toString(_owner.serial()));
    const auto thisThreadHasTheLock = _pathfindingMutex.try_lock();
    if (!thisThreadHasTheLock) return;
    Server::instance().incrementThreadCount();

    const auto distToTravel = distance(_owner.collisionRect(), targetFootprint);
    const auto startTime = SDL_GetTicks();

    _activePath.findPathTo(targetFootprint);

    const auto endTime = SDL_GetTicks();
    const auto timeElapsed = endTime - startTime;
    auto of = std::ofstream{"pathfinding.log", std::ios_base::app};
    of << _owner.type()->id()  // NPC ID
       << "," << distToTravel  // Straight-line distance
       << "," << timeElapsed   // Pathfinding time
       << std::endl;

    _pathfindingMutex.unlock();
    Server::instance().decrementThreadCount();
  }).detach();
}

bool AI::targetHasMoved() const {
  if (!_activePath.exists()) return false;

  const auto pathEnd = _activePath.lastWaypoint();
  const auto targetLocation = getTargetFootprint();

  const auto MAX_TARGET_MOVEMENT_BEFORE_REPATH = 30.0;
  const auto maxDistanceAllowed =
      MAX_TARGET_MOVEMENT_BEFORE_REPATH + howCloseShouldPathfindingGet();
  const auto gapBetweenTargetAndPathEnd = distance(pathEnd, targetLocation);
  return gapBetweenTargetAndPathEnd > maxDistanceAllowed;
}

MapRect AI::getTargetFootprint() const {
  if (state == AI::CHASE) return _owner.target()->collisionRect();
  if (state == AI::PET_FOLLOW_OWNER)
    return _owner.followTarget()->collisionRect();

  SERVER_ERROR("Pathfinding while AI is in an inappropriate state");
  return {};
}

double AI::howCloseShouldPathfindingGet() const {
  if (state == AI::CHASE) return _owner.attackRange();
  if (state == AI::PET_FOLLOW_OWNER) return AI::FOLLOW_DISTANCE;

  SERVER_ERROR("Pathfinding while AI is in an inappropriate state");
  return 0;
}
