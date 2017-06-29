#include "TestServer.h"
#include "testing.h"

TEST_CASE("nearbyTerrainTypes without radius", "[terrain]"){
    TestServer s = TestServer::WithData("terrain_medley");
    auto set = s->nearbyTerrainTypes(Rect(0, 0, 40, 40));
    CHECK (set.count('a') == 1);
    CHECK (set.count('b') == 1);
    CHECK (set.count('c') == 0);
    CHECK (set.count('.') == 0);
}

TEST_CASE("nearbyTerrainTypes with radius", "[terrain]"){
    TestServer s = TestServer::WithData("terrain_medley");
    auto set = s->nearbyTerrainTypes(Rect(0, 0, 40, 40), 200);
    CHECK(set.count('a') == 1);
    CHECK(set.count('b') == 1);
    CHECK(set.count('c') == 1);
    CHECK(set.count('.') == 0);
}

TEST_CASE("Large map is read accurately", "[.slow][terrain]"){
    TestServer s = TestServer::WithData("signpost");
    REQUIRE(s->mapX() == 315);
    REQUIRE(s->mapY() == 315);
    for (size_t x = 0; x != 315; ++x)
        for (size_t y = 0; y != 315; ++y){
            CAPTURE(x);
            CAPTURE(y);
            REQUIRE(s->map()[x][y] == 'G');
        }
}
