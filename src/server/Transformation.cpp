#include "Transformation.h"

#include "Entity.h"
#include "Objects/ObjectType.h"
#include "objects/Object.h"

void Transformation::update(ms_t timeElapsed) {
  if (parent().isDead()) return;
  if (_transformTimer == 0) return;

  const auto *parentType = dynamic_cast<const ObjectType *>(parent().type());
  if (!parentType) return;
  if (!parentType->transformation.transformObject) return;
  if (parentType->transformation.transformOnEmpty &&
      parent().gatherable.hasItems())
    return;

  if (timeElapsed > _transformTimer)
    _transformTimer = 0;
  else
    _transformTimer -= timeElapsed;
  if (_transformTimer > 0) return;

  auto &asObj = dynamic_cast<Object &>(parent());
  asObj.setType(parentType->transformation.transformObject,
                parentType->transformation.skipConstructionOnTransform);
}

void Transformation::initialise() {
  const auto *objType = dynamic_cast<const ObjectType *>(parent().type());
  if (!objType->transformation.transformObject) return;
  _transformTimer = objType->transformation.transformTime;
}
