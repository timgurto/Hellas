#include "Transformation.h"

#include "Entity.h"
#include "Objects/ObjectType.h"
#include "objects/Object.h"

void Transformation::update(ms_t timeElapsed) {
  if (parent().isDead()) return;
  if (_timeUntilTransform == 0) return;

  if (!parent().type()->transformation.newType) return;
  if (parent().type()->transformation.mustBeGathered &&
      parent().gatherable.hasItems())
    return;

  if (timeElapsed > _timeUntilTransform)
    _timeUntilTransform = 0;
  else
    _timeUntilTransform -= timeElapsed;
  if (_timeUntilTransform > 0) return;

  auto *asObj = dynamic_cast<Object *>(&parent());
  if (!asObj) return;
  asObj->setType(parent().type()->transformation.newType,
                 parent().type()->transformation.becomesFullyConstructed);
}

void Transformation::initialise() {
  if (!parent().type()->transformation.newType) return;
  _timeUntilTransform = parent().type()->transformation.delay;
}
