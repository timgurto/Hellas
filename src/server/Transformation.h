#pragma once

#include "../types.h"
#include "EntityComponent.h"

class EntityType;
class XmlReader;
class TiXmlElement;

class Transformation : public EntityComponent {
 public:
  Transformation(Entity &parent) : EntityComponent(parent) {}

  bool isTransforming() const { return _timeUntilTransform > 0; }
  ms_t transformTimer() const { return _timeUntilTransform; }
  void transformTimer(ms_t timeRemaining) {
    _timeUntilTransform = timeRemaining;
  }

  void update(ms_t timeElapsed);
  void initialise();

 private:
  ms_t _timeUntilTransform{0};  // When this hits zero, it switches types.
};

struct TransformationType {
  const EntityType *newType{nullptr};
  ms_t delay{0};
  bool mustBeGathered{false};
  bool becomesFullyConstructed{false};

  template <typename T>
  void loadFromXML(XmlReader &xr, TiXmlElement *elem) {
    auto e = xr.findChild("transform", elem);
    if (!e) return;

    auto &server = Server::instance();

    auto id = ""s;
    if (!xr.findAttr(e, "id", id)) {
      server._debug("Transformation specified without target id; skipping.",
                    Color::CHAT_ERROR);
      return;
    }
    const auto *transformObjPtr = server.findObjectTypeByID(id);
    if (!transformObjPtr) {
      transformObjPtr = new T(id);
      server.addObjectType(transformObjPtr);
    }
    newType = transformObjPtr;
    xr.findAttr(e, "time", delay);

    auto n = 0;
    if (xr.findAttr(e, "whenEmpty", n) && n != 0) mustBeGathered = true;
    if (xr.findAttr(e, "skipConstruction", n) && n != 0)
      becomesFullyConstructed = true;
  }
};
