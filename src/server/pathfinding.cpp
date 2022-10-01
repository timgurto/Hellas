#include "pathfinding.h"
#include "../threadNaming.h"
#include "NPC.h"
#include "Server.h"

void Pathfinder::Path::findPathTo(const MapRect &targetFootprint) {
  // A*
  const auto GRID = 25.0;
  static const auto DIAG = sqrt(GRID * GRID + GRID * GRID);
  const auto closeEnough = _owningPathfinder.howCloseShouldPathfindingGet();
  const auto footprint = owningNPC().type()->collisionRect();

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
  const auto startPoint = owningNPC().location();
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
                                           owningNPC())) {
      _queue = tracePathTo(bestCandidatePoint);
      _queue.push(targetFootprint);
      return;
    }

    // Calculate F, set to this if existing entry is higher or missing
    for (const auto &extension : extensionCandidates) {
      const auto nextPoint = bestCandidatePoint + extension.delta;
      auto stepRect =
          footprint + bestCandidatePoint + extension.journeyRectDelta();
      if (Server::instance().isLocationValid(stepRect, owningNPC())) {
        const auto currentNode = nodesByPoint[bestCandidatePoint];
        auto nextNode = AStarNode{};
        nextNode.parentInBestPath = bestCandidatePoint;
        nextNode.g = currentNode.g + extension.distance;
        const auto h = distance(footprint + nextPoint, targetFootprint);

        const auto pathStraysTooFar =
            h > owningNPC().ai.PURSUIT_RANGE &&
            !owningNPC().npcType()->pursuesEndlessly();
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

void Pathfinder::calculatePathInSeparateThread() {
  const auto targetFootprint = getTargetFootprint();

  std::thread([this, targetFootprint]() {
    setThreadName("Pathfinding for " + _owningNPC.type()->id() + " serial " +
                  toString(_owningNPC.serial()));
    const auto thisThreadHasTheLock = _pathfindingMutex.try_lock();
    if (!thisThreadHasTheLock) return;
    Server::instance().incrementThreadCount();

    const auto distToTravel =
        distance(_owningNPC.collisionRect(), targetFootprint);

    _activePath.findPathTo(targetFootprint);
    if (!_activePath.exists()) _failedToFindPath = true;

    _pathfindingMutex.unlock();
    Server::instance().decrementThreadCount();
  }).detach();
}
