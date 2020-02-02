#pragma once

#include "../types.h"
#include "EntityComponent.h"

class ObjectType;
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
  const ObjectType *newType{nullptr};
  ms_t delay{0};
  bool mustBeGathered{false};
  bool becomesFullyConstructed{false};

  void loadFromXML(XmlReader &xr, TiXmlElement *elem);
};
