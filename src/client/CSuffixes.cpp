#include "CSuffixes.h"

std::string CSuffixSets::getSuffixName(std::string suffixSetID,
                                       std::string suffixID) const {
  auto it = sets.find(suffixSetID);
  if (it == sets.end()) return {};
  const auto &set = it->second;

  auto suffixIt = set.find(suffixID);
  if (suffixIt == set.end()) return {};

  return suffixIt->second.name;
}

const StatsMod &CSuffixSets::getSuffixStats(std::string suffixSetID,
                                            std::string suffixID) const {
  static const auto dummy = StatsMod{};

  auto it = sets.find(suffixSetID);
  if (it == sets.end()) return dummy;
  const auto &set = it->second;

  auto suffixIt = set.find(suffixID);
  if (suffixIt == set.end()) return dummy;

  return suffixIt->second.stats;
}
