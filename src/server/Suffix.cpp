#include "Suffix.h"

StatsMod SuffixSets::chooseRandomSuffix(std::string setID) const {
  const auto &suffixPool = suffixStats.find(setID)->second;
  const auto index = rand() % suffixPool.size();
  return suffixPool[index];
}
