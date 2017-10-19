#pragma once

#include <SDL.h>

// Pixels
typedef int px_t;
px_t operator"" _px(unsigned long long x);

// Time, in milliseconds
typedef Uint32 ms_t;

// Hitpoints
typedef unsigned health_t;
