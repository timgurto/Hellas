#pragma once

#include <map>

struct CSuffixSets {
  struct Suffix {
    std::string name;
  };
  using SuffixSet = std::map<std::string, Suffix>;
  std::map<std::string, SuffixSet> sets;
  std::string getSuffixName(std::string suffixSetID,
                            std::string suffixID) const;
};
