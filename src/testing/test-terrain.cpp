#include "TestServer.h"
#include "testing.h"

TEST_CASE("nearbyTerrainTypes without radius"){
    TestServer s = TestServer::WithData("terrain_medley");
    auto set = s->nearbyTerrainTypes(Rect(0, 0, 40, 40));
    CHECK (set.count('a') == 1);
    CHECK (set.count('b') == 1);
    CHECK (set.count('c') == 0);
    CHECK (set.count('.') == 0);
}

TEST_CASE("nearbyTerrainTypes with radius"){
    TestServer s = TestServer::WithData("terrain_medley");
    auto set = s->nearbyTerrainTypes(Rect(0, 0, 40, 40), 200);
    CHECK(set.count('a') == 1);
    CHECK(set.count('b') == 1);
    CHECK(set.count('c') == 1);
    CHECK(set.count('.') == 0);
}
