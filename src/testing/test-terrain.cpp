#include "Test.h"
#include "ServerTestInterface.h"

ONLY_TEST("nearbyTerrainTypes without radius")
    ServerTestInterface s;
    s.loadData("testing/data/terrain_medley");
    auto set = s->nearbyTerrainTypes(Rect(0, 0, 40, 40));
    if (set.count('a') == 0) return false;
    if (set.count('b') == 0) return false;
    if (set.count('c') == 1) return false;
    if (set.count('.') == 1) return false;
    return true;
TEND

ONLY_TEST("nearbyTerrainTypes with radius")
    ServerTestInterface s;
    s.loadData("testing/data/terrain_medley");
    auto set = s->nearbyTerrainTypes(Rect(0, 0, 40, 40), 200);
    if (set.count('a') == 0) return false;
    if (set.count('b') == 0) return false;
    if (set.count('c') == 0) return false;
    if (set.count('.') == 1) return false;
    return true;
TEND
