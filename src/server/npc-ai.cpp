#include "NPC.h"
#include "Server.h"

void NPC::order(Order newOrder) {
  _order = newOrder;
  _homeLocation = location();

  // Send order confirmation to owner
  auto owner = permissions.getPlayerOwner();
  if (!owner) return;
  auto serialArg = makeArgs(serial());
  switch (_order) {
    case NPC::STAY:
      owner->sendMessage({SV_PET_IS_NOW_STAYING, serialArg});
      break;

    case NPC::FOLLOW:
      owner->sendMessage({SV_PET_IS_NOW_FOLLOWING, serialArg});
      break;

    default:
      break;
  }
}

void NPC::getNewTargetsFromProximity(ms_t timeElapsed) {
  auto shouldLookForNewTargetsNearby =
      npcType()->attacksNearby() || permissions.hasOwner();
  if (!shouldLookForNewTargetsNearby) return;
  _timeSinceLookedForTargets += timeElapsed;
  if (_timeSinceLookedForTargets < AI::FREQUENCY_TO_LOOK_FOR_TARGETS) return;
  _timeSinceLookedForTargets =
      _timeSinceLookedForTargets % AI::FREQUENCY_TO_LOOK_FOR_TARGETS;
  for (auto *potentialTarget :
       Server::_instance->findEntitiesInArea(location(), AI::AGGRO_RANGE)) {
    if (potentialTarget == this) continue;
    if (!potentialTarget->canBeAttackedBy(*this)) continue;
    if (potentialTarget->shouldBeIgnoredByAIProximityAggro()) continue;
    if (distance(collisionRect(), potentialTarget->collisionRect()) >
        AI::AGGRO_RANGE)
      continue;
    makeAwareOf(*potentialTarget);
  }
}

void NPC::setStateBasedOnOrder() {
  if (_order == STAY)
    ai.state = AI::IDLE;
  else
    ai.state = AI::PET_FOLLOW_OWNER;
}
