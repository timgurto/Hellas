#pragma once

#include "../Stats.h"

struct SuffixSets {
  struct Suffix {
    std::string id;
    StatsMod stats;
  };

  std::map<std::string, std::map<std::string, Suffix> > suffixStats;

  const Suffix& chooseRandomSuffix(std::string setID) const;
  StatsMod getStatsForSuffix(std::string setID, std::string suffixID);
};
