#pragma once
#include <SDL.h>

// Pixels
typedef int px_t;
px_t operator"" _px(unsigned long long x);

// Time, in milliseconds
typedef Uint32 ms_t;

// Combat
using Hitpoints = unsigned;
using Energy = unsigned;
using Percentage = short;
using BonusDamage = int;
using Regen = int;

using Level = short;
using XP = unsigned;

// Additions to this enum must be backwards-compatible, so as not to invalidate
// users' saved hotbars from older versions.
enum HotbarCategory { HOTBAR_NONE = 0, HOTBAR_SPELL = 1, HOTBAR_RECIPE = 2 };
