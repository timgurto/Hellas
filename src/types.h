#pragma once
#include <SDL.h>

#include <string>

// Pixels
typedef int px_t;
px_t operator"" _px(unsigned long long x);

// Time, in milliseconds
typedef Uint32 ms_t;
using XP = unsigned;

// Additions to this enum must be backwards-compatible, so as not to invalidate
// users' saved hotbars from older versions.
enum HotbarCategory { HOTBAR_NONE = 0, HOTBAR_SPELL = 1, HOTBAR_RECIPE = 2 };

struct RepairInfo {
  bool canBeRepaired{false};

  std::string cost{};
  bool hasCost() const { return !cost.empty(); }

  std::string tool{};
  bool requiresTool() const { return !tool.empty(); }
};

using Filename = std::string;
using FilenameWithoutSuffix = std::string;
using Username = std::string;
