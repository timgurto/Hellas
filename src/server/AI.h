#pragma once

#include <mutex>
#include <queue>

#include "../Point.h"
#include "../types.h"
#include "pathfinding.h"

class AI : public Pathfinder {
 public:
  static const px_t AGGRO_RANGE{70};
  static const px_t PURSUIT_RANGE{245};  // Assumption: this is farther than any
                                         // ranged attack/spell can reach:
  static const px_t FOLLOW_DISTANCE{28};
  static const px_t MAX_FOLLOW_RANGE{210};
  static const ms_t FREQUENCY_TO_LOOK_FOR_TARGETS{250};

  AI(class NPC &owner);

  enum State { IDLE, CHASE, ATTACK, PET_FOLLOW_OWNER, RETREAT } state{IDLE};

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

  MapRect getTargetFootprint() const override;
  double howCloseShouldPathfindingGet() const override;
  bool isDistanceTooFarToPathfind(double distance) const override;
  std::string threadName() const override;
  void pickRandomSpotNearSpawnPoint();
};
