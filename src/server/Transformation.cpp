#include "Transformation.h"

#include "Entity.h"
#include "Objects/ObjectType.h"
#include "Server.h"
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

void TransformationType::loadFromXML(XmlReader &xr, TiXmlElement *elem) {
  auto e = xr.findChild("transform", elem);
  if (!e) return;

  auto &server = Server::instance();

  auto id = ""s;
  if (!xr.findAttr(e, "id", id)) {
    server._debug("Transformation specified without target id; skipping.",
                  Color::CHAT_ERROR);
    return;
  }
  const ObjectType *transformObjPtr = server.findObjectTypeByID(id);
  if (!transformObjPtr) {
    transformObjPtr = new ObjectType(id);
    server.addObjectType(transformObjPtr);
  }
  newType = transformObjPtr;
  xr.findAttr(e, "time", delay);

  auto n = 0;
  if (xr.findAttr(e, "whenEmpty", n) && n != 0) mustBeGathered = true;
  if (xr.findAttr(e, "skipConstruction", n) && n != 0)
    becomesFullyConstructed = true;
}
