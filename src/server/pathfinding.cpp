#include "pathfinding.h"
#include "../threadNaming.h"
#include "NPC.h"
#include "Server.h"

void Pathfinder::Path::findPathTo(const MapRect &targetFootprint) {
  // A*
  const auto GRID = 25.0;
  static const auto DIAG = sqrt(GRID * GRID + GRID * GRID);
  const auto closeEnough = _owningPathfinder.howCloseShouldPathfindingGet();
  const auto footprint = owningEntity().type()->collisionRect();

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
  const auto startPoint = owningEntity().location();
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
  const auto extensionCandidates = std::vector<Extension>{
      // clang-format off
    {{+GRID, -GRID}, DIAG},
    {{+GRID, +GRID}, DIAG},
    {{-GRID, +GRID}, DIAG},
    {{-GRID, -GRID}, DIAG},
    {{    0, -GRID}, GRID},
    {{    0, +GRID}, GRID },
    {{-GRID,     0}, GRID },
    {{+GRID,     0}, GRID }
      // clang-format on
  };

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
    if (Server::instance().isLocationValid(journeyRectToDestination,
                                           owningEntity())) {
      _queue = tracePathTo(bestCandidatePoint);
      _queue.push(targetFootprint);
      return;
    }

    // Calculate F, set to this if existing entry is higher or missing
    for (const auto &extension : extensionCandidates) {
      const auto nextPoint = bestCandidatePoint + extension.delta;
      auto stepRect =
          footprint + bestCandidatePoint + extension.journeyRectDelta();
      if (Server::instance().isLocationValid(stepRect, owningEntity())) {
        const auto currentNode = nodesByPoint[bestCandidatePoint];
        auto nextNode = AStarNode{};
        nextNode.parentInBestPath = bestCandidatePoint;
        nextNode.g = currentNode.g + extension.distance;
        const auto h = distance(footprint + nextPoint, targetFootprint);
        if (_owningPathfinder.isDistanceTooFarToPathfind(h)) continue;

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

bool Pathfinder::targetHasMoved() const {
  if (!_activePath.exists()) return false;

  const auto pathEnd = MapRect{_activePath.lastWaypoint()};
  const auto targetLocation = getTargetFootprint();

  const auto MAX_TARGET_MOVEMENT_BEFORE_REPATH = 30.0;
  const auto maxDistanceAllowed =
      MAX_TARGET_MOVEMENT_BEFORE_REPATH + howCloseShouldPathfindingGet();
  const auto gapBetweenTargetAndPathEnd = distance(pathEnd, targetLocation);
  return gapBetweenTargetAndPathEnd > maxDistanceAllowed;
}

void Pathfinder::calculatePathInSeparateThread() {
  const auto targetFootprint = getTargetFootprint();

  std::thread([this, targetFootprint]() {
    setThreadName(threadName());
    const auto thisThreadHasTheLock = _pathfindingMutex.try_lock();
    if (!thisThreadHasTheLock) return;
    Server::instance().incrementThreadCount();

    const auto distToTravel =
        distance(_owningEntity->collisionRect(), targetFootprint);

    _activePath.findPathTo(targetFootprint);
    if (!_activePath.exists()) _failedToFindPath = true;

    _pathfindingMutex.unlock();
    Server::instance().decrementThreadCount();
  }).detach();
}

void Pathfinder::progressPathfinding() {
  if (!_activePath.exists()) {
    calculatePathInSeparateThread();
    return;
  }

  // Move towards target
  if (targetHasMoved()) {
    calculatePathInSeparateThread();
    return;
  }

  // If reached the current waypoint, move to the next one
  if (_owningEntity->location() == _activePath.currentWaypoint())
    _activePath.changeToNextWaypoint();
  if (!_activePath.exists()) return;

  const auto result =
      _owningEntity->moveLegallyTowards(_activePath.currentWaypoint());
  if (result == Entity::MOVED_INTO_OBSTACLE) calculatePathInSeparateThread();
}
