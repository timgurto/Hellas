#pragma once

#include <string>
#include <vector>

#include "../../../src/Point.h"

struct StaticObject {
  enum Type { OBJECT, NPC };
  Type type;
  std::string id;
  MapPoint loc;

  using Container = std::vector<StaticObject>;
  static void load(Container &container, const std::string &filename);
};
