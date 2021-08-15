#include "CSuffixes.h"

std::string CSuffixSets::getSuffixName(std::string suffixSetID,
                                       std::string suffixID) const {
  auto it = sets.find(suffixSetID);
  if (it == sets.end()) return {};
  const auto &set = it->second;

  auto suffixIt = set.find(suffixID);
  if (suffixIt == set.end()) return {};

  return suffixIt->second;
}
