#pragma once

#include "../Stats.h"

struct SuffixSets {
  struct Suffix {
    StatsMod stats;
  };

  std::map<std::string, std::map<std::string, Suffix> > suffixStats;

  std::string chooseRandomSuffix(std::string setID) const;
  StatsMod getStatsForSuffix(std::string setID, std::string suffixID);
};
