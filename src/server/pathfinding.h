#pragma once

#include <mutex>
#include <queue>

#include "../Point.h"
#include "../Rect.h"

class Entity;

class Pathfinder {
 public:
  Pathfinder(Entity &owningEntity)
      : _owningEntity(owningEntity), _activePath(*this) {}

  virtual void progressPathfinding();

 protected:
  std::mutex _pathfindingMutex;
  bool _failedToFindPath{false};

  void calculatePathInSeparateThread();
  bool targetHasMoved() const;
  virtual MapRect getTargetFootprint() const = 0;
  virtual double howCloseShouldPathfindingGet() const = 0;
  virtual bool isDistanceTooFarToPathfind(double dist) const = 0;
  virtual std::string threadName() const { return "Pathfinding"; }

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
    const Entity &owningEntity() const {
      return _owningPathfinder._owningEntity;
    }
  } _activePath;

  Entity &_owningEntity;
};
