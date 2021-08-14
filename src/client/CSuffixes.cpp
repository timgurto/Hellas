#include "CSuffixes.h"

std::string CSuffixSets::getSuffixName(std::string suffixSetID) const {
  auto it = setIDToSuffixName.find(suffixSetID);
  if (it == setIDToSuffixName.end()) return {};
  return it->second;
}
