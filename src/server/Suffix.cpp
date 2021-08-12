#include "Suffix.h"

std::string SuffixSets::chooseRandomSuffix(std::string setID) const {
  const auto &suffixSet = suffixStats.find(setID)->second;
  const auto chosenSuffix = rand() % suffixSet.size();

  auto it = suffixSet.begin();
  for (auto i = 0; i != chosenSuffix; ++i) ++it;
  return it->first;
}

StatsMod SuffixSets::getStatsForSuffix(std::string setID,
                                       std::string suffixID) {
  return suffixStats[setID][suffixID].stats;
}
