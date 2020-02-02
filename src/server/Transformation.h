#pragma once

#include "../types.h"
#include "EntityComponent.h"

class ObjectType;

class Transformation : public EntityComponent {
 public:
  Transformation(Entity &parent) : EntityComponent(parent) {}

  bool isTransforming() const { return _transformTimer > 0; }
  ms_t transformTimer() const { return _transformTimer; }
  void transformTimer(ms_t timeRemaining) { _transformTimer = timeRemaining; }

  void update(ms_t timeElapsed);
  void initialise();

 private:
  ms_t _transformTimer{0};  // When this hits zero, it switches types.
};

struct TransformationType {
  const ObjectType *transformObject =
      nullptr;  // The object type that this becomes over time, if any.
  ms_t transformTime = 0;         // How long the transformation takes.
  bool transformOnEmpty = false;  // Begin the transformation only once all
                                  // items have been gathered.
  bool skipConstructionOnTransform =
      false;  // If true, the new object will be fully constructed
};
