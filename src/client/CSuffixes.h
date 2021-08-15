#pragma once

#include <map>

struct CSuffixSets {
  using SuffixSet = std::map<std::string, std::string>;  // ID -> name
  std::map<std::string, SuffixSet> sets;
  std::string getSuffixName(std::string suffixSetID,
                            std::string suffixID) const;
};
