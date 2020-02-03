#include "Transformation.h"

#include "Entity.h"
#include "EntityType.h"
#include "Server.h"
#include "objects/Object.h"

void Transformation::update(ms_t timeElapsed) {
  if (parent().isDead()) return;
  if (_timeUntilTransform == 0) return;

  auto transforms = parent().type()->transformation.newType != nullptr;
  if (!transforms) return;

  auto blockedUntilGathered = parent().type()->transformation.mustBeGathered &&
                              parent().gatherable.hasItems();
  if (blockedUntilGathered) return;

  if (parent().classTag() == 'n' && !parent().permissions.hasOwner()) return;

  if (timeElapsed > _timeUntilTransform)
    _timeUntilTransform = 0;
  else
    _timeUntilTransform -= timeElapsed;
  if (_timeUntilTransform > 0) return;

  parent().changeType(parent().type()->transformation.newType,
                      parent().type()->transformation.becomesFullyConstructed);
}

void Transformation::initialise() {
  if (!parent().type()->transformation.newType) return;
  _timeUntilTransform = parent().type()->transformation.delay;
}
