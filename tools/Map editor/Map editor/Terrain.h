#pragma once

#include <map>

#include "../../../src/XmlReader.h"

struct TerrainType {
  char index{'\0'};
  std::string id;
  Color color;

  using Container = std::map<char, TerrainType>;
  static void load(Container &container, const std::string &filename);
};
