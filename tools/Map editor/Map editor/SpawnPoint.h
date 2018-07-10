#pragma once

#include <string>
#include <vector>

#include "../../../src/Point.h"

struct SpawnPoint {
  enum Type { OBJECT, NPC };
  Type type;
  std::string id;
  MapPoint loc;
  int radius{0};
  int quantity{0};
  ms_t respawnTime{0};

  using Container = std::vector<SpawnPoint>;
  static void load(Container &container, const std::string &filename);
};
