#pragma once

#include <set>
#include <string>
#include <vector>

#include "../../../src/Point.h"
#include "EntityType.h"

struct SpawnPoint {
  std::string id;
  MapPoint loc;
  int radius{0};
  int quantity{0};
  ms_t respawnTime{0};
  bool useCachedTerrain{false};

  bool operator<(const SpawnPoint &rhs) const;

  using Container = std::set<SpawnPoint>;
  static void load(Container &container, const std::string &filename);
  static void save(const Container &container, std::string xmlOutputFilename,
                   std::string csvNPCsOutputFilename,
                   EntityType::Container &entityTypes);
};
