#pragma once

#include <queue>

#include "../Point.h"
#include "../types.h"

class AI {
 public:
  static const px_t AGGRO_RANGE{70};
  static const px_t PURSUIT_RANGE{245};  // Assumption: this is farther than any
                                         // ranged attack/spell can reach:
  static const px_t FOLLOW_DISTANCE{28};
  static const px_t MAX_FOLLOW_RANGE{210};
  static const ms_t FREQUENCY_TO_LOOK_FOR_TARGETS{250};

  AI(class NPC &owner);

  enum State { IDLE, CHASE, ATTACK, PET_FOLLOW_OWNER } state{IDLE};

  enum PetOrder {
    ORDER_TO_STAY,
    ORDER_TO_FOLLOW
  } order{ORDER_TO_FOLLOW};  // A desire; informs pet actions

  void process(ms_t timeElapsed);

  void giveOrder(AI::PetOrder newOrder);
  PetOrder currentOrder() const { return order; }

 private:
  NPC &_owner;

  MapPoint _homeLocation;  // Where it returns after a chase.

  void transitionIfNecessary();
  void onTransition(AI::State previousState);
  void act();

  class Path {
   public:
    Path(const NPC &owner) : _owner(owner) {}
    MapPoint currentWaypoint() const { return _queue.front(); }
    void changeToNextWaypoint() { _queue.pop(); }
    void findIndirectPathTo(const MapPoint &destination);
    void findDirectPathTo(const MapPoint &destination);
    void clear() { _queue = {}; }

   private:
    std::queue<MapPoint> _queue;
    const NPC &_owner;
  } _path;
};
