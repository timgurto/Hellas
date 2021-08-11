#pragma once

#include "../Stats.h"

struct SuffixSets {
  std::map<std::string, std::vector<StatsMod> > suffixStats;

  StatsMod chooseRandomSuffix(std::string setID) const;
};
