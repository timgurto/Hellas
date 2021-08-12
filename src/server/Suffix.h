#pragma once

#include "../Stats.h"

struct SuffixSets {
  std::map<std::string, std::map<std::string, StatsMod> > suffixStats;

  std::string chooseRandomSuffix(std::string setID) const;
  StatsMod getStatsForSuffix(std::string setID, std::string suffixID);
};
