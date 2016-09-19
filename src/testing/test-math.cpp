// (C) 2016 Tim Gurto

#include <cstdlib>
#include <SDL.h>

#include "Test.h"
#include "../Color.h"

TEST("Convert from Uint32 to Color and back")
    Uint32 testNum = rand();
    Color c = testNum;
    Uint32 result = c;
    return testNum == result;
TEND
