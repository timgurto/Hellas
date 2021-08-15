#pragma once

#include <map>
#include "..\Stats.h"

struct CSuffixSets {
  struct Suffix {
    std::string name;
  };
  using SuffixSet = std::map<std::string, Suffix>;
  std::map<std::string, SuffixSet> sets;
  std::string getSuffixName(std::string suffixSetID,
                            std::string suffixID) const;
  const StatsMod& getSuffixStats(std::string suffixSetID,
                                 std::string suffixID) const;
};
