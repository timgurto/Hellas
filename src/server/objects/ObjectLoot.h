#ifndef OBJECT_LOOT_H
#define OBJECT_LOOT_H

#include "../Loot.h"

class Object;

class ObjectLoot : public Loot {
 public:
  ObjectLoot(Object &parent);
  void populate();

 private:
  Object &_parent;

  void addContainerItemsToLoot();
};

#endif
