#pragma once

class NPC;

class Pathfinder {
 public:
  Pathfinder(NPC &owningNPC) : _owningNPC(owningNPC), _activePath(*this) {}

 protected:
  std::mutex _pathfindingMutex;
  bool _failedToFindPath{false};

  virtual void calculatePathInSeparateThread() = 0;
  virtual double howCloseShouldPathfindingGet() const = 0;

  class Path {
   public:
    Path(const Pathfinder &owningPathfinder)
        : _owningPathfinder(owningPathfinder) {}
    MapPoint currentWaypoint() const { return _queue.front(); }
    MapPoint lastWaypoint() const { return _queue.back(); }
    void changeToNextWaypoint() { _queue.pop(); }
    void findPathTo(const MapRect &destinationFootprint);
    void clear() { _queue = {}; }
    bool exists() const { return !_queue.empty(); }

   private:
    std::queue<MapPoint> _queue;
    const Pathfinder &_owningPathfinder;
    const NPC &owningNPC() const { return _owningPathfinder._owningNPC; }
  } _activePath;

  NPC &_owningNPC;
};
