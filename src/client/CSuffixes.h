#pragma once

#include <map>

struct CSuffixSets {
  std::map<std::string, std::string> setIDToSuffixName;
  std::string getSuffixName(std::string suffixSetID) const;
};
