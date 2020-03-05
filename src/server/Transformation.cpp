#include "Transformation.h"

#include "Entity.h"
#include "EntityType.h"
#include "Server.h"
#include "objects/Object.h"

void Transformation::update(ms_t timeElapsed) {
  // Skip if dead
  if (!_transforms) return;
  if (parent().isDead()) return;

  auto blockedUntilGathered = parent().type()->transformation.mustBeGathered &&
                              parent().gatherable.hasItems();
  if (blockedUntilGathered) return;

  // Skip if unowned NPC
  if (parent().classTag() == 'n' && !parent().permissions.hasOwner()) return;

  // Check timer
  if (timeElapsed > _timeUntilTransform)
    _timeUntilTransform = 0;
  else
    _timeUntilTransform -= timeElapsed;
  if (_timeUntilTransform > 0) return;

  parent().changeType(parent().type()->transformation.newType,
                      parent().type()->transformation.becomesFullyConstructed);
}

void Transformation::initialise() {
  const auto &type = parent().type()->transformation;
  _transforms = type.transforms && type.newType;
  if (!_transforms) return;
  _timeUntilTransform = type.delay;
}
