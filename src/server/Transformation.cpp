#include "Transformation.h"

#include "Entity.h"
#include "Objects/ObjectType.h"
#include "objects/Object.h"

void Transformation::update(ms_t timeElapsed) {
  if (parent().isDead()) return;
  if (_transformTimer == 0) return;

  if (!parent().type()->transformation.transformObject) return;
  if (parent().type()->transformation.transformOnEmpty &&
      parent().gatherable.hasItems())
    return;

  if (timeElapsed > _transformTimer)
    _transformTimer = 0;
  else
    _transformTimer -= timeElapsed;
  if (_transformTimer > 0) return;

  auto *asObj = dynamic_cast<Object *>(&parent());
  if (!asObj) return;
  asObj->setType(parent().type()->transformation.transformObject,
                 parent().type()->transformation.skipConstructionOnTransform);
}

void Transformation::initialise() {
  if (!parent().type()->transformation.transformObject) return;
  _transformTimer = parent().type()->transformation.transformTime;
}
