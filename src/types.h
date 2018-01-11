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
