#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("nearbyTerrainTypes without radius") {
  TestServer s = TestServer::WithData("terrain_medley");
  auto set = s->map().terrainTypesOverlapping({0, 0, 40, 40});
  CHECK(set.count('a') == 1);
  CHECK(set.count('b') == 1);
  CHECK(set.count('c') == 0);
  CHECK(set.count('.') == 0);
}

TEST_CASE("nearbyTerrainTypes with radius") {
  TestServer s = TestServer::WithData("terrain_medley");
  auto set = s->map().terrainTypesOverlapping({0, 0, 40, 40}, 200);
  CHECK(set.count('a') == 1);
  CHECK(set.count('b') == 1);
  CHECK(set.count('c') == 1);
  CHECK(set.count('.') == 0);
}

TEST_CASE("Large map is read accurately", "[.slow][loading]") {
  TestServer s = TestServer::WithData("signpost");
  REQUIRE(s->map().width() == 315);
  REQUIRE(s->map().height() == 315);
  for (size_t x = 0; x != 315; ++x)
    for (size_t y = 0; y != 315; ++y) {
      CAPTURE(x);
      CAPTURE(y);
      REQUIRE(s->map()[x][y] == 'G');
    }
}

TEST_CASE("Users limited to default terrain list", "[allowed-terrain]") {
  GIVEN("a map with grass and water, and only grass in the default list") {
    auto data = R"(
      <terrain index="G" id="grass" />
      <terrain index="." id="water" />
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="4" y="4" />
      <row    y="0" terrain = "GG.." />
      <row    y="1" terrain = "GG.." />
      <row    y="2" terrain = "...." />
      <row    y="3" terrain = "...." />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("he tries to walk onto the water") {
      REPEAT_FOR_MS(2000) {
        c.sendMessage(CL_MOVE_TO, makeArgs(70, 10));
        SDL_Delay(5);
      }

      THEN("he can't get there") { CHECK(user.location().x != 70.0); }
    }
  }
}

TEST_CASE("Client objects' allowed-terrain sets", "[allowed-terrain]") {
  GIVEN("an object type allowed on non-default terrain") {
    auto data = R"(
      <objectType id="boat" allowedTerrain="water" />
      <objectType id="frog" allowedTerrain="anything" />
      <terrain index="w" id="water" />
      <list id="water" >
          <allow id="water" />
      </list>
      <list id="anything" >
          <allow id="water" />
          <allow id="grass" />
      </list>
    )";
    auto c = TestClient::WithDataString(data);
    const auto &boat = *c->findObjectType("boat");
    const auto &frog = *c->findObjectType("frog");

    THEN("the client knows its valid terrain") {
      CHECK(boat.allowedTerrain() == "water");
      CHECK(frog.allowedTerrain() == "anything");
    }
  }
}

TEST_CASE("Terrain-set descriptions", "[allowed-terrain][ui]") {
  GIVEN("terrain lists with descriptions") {
    auto data = R"(
      <list id="land" description="Suitable on land." />
      <list id="water" description="Suitable on water." />
    )";
    auto s = TestClient::WithDataString(data);

    THEN("the descriptions are loaded") {
      CHECK(TerrainList::description("land") == "Suitable on land.");
      CHECK(TerrainList::description("water") == "Suitable on water.");
    }
  }

  GIVEN("a list with no description") {
    auto data = R"(
      <list id="noDescription" />
    )";
    auto s = TestServer::WithDataString(data);

    THEN("it is still loaded") {
      CHECK(TerrainList::findList("noDescription") != nullptr);
    }
  }

  GIVEN("a server") {
    auto s = TestServer{};

    WHEN("the description of a nonexistent list is looked up") {
      auto description = TerrainList::description("nonexistent");

      THEN("the server doesn't crash") { CHECK(description == description); }
    }
  }
}

TEST_CASE("randomPointInTile()") {
  SECTION("Simple tiles") {
    for (auto i = 0; i != 20; ++i) {
      CHECK(collision(Map::randomPointInTile(0, 0), {-16.0, 0, 32.0, 32.0}));
      CHECK(collision(Map::randomPointInTile(1, 0), {16.0, 0, 32.0, 32.0}));
      CHECK(collision(Map::randomPointInTile(2, 0), {48.0, 0, 32.0, 32.0}));
      CHECK(collision(Map::randomPointInTile(0, 2), {-16.0, 64.0, 32.0, 32.0}));
      CHECK(collision(Map::randomPointInTile(0, 1), {0, 32.0, 32.0, 32.0}));
    }
  }

  SECTION("Results are random") {
    CHECK(Map::randomPointInTile(0, 0) != Map::randomPointInTile(0, 0));
  }
}

TEST_CASE("Transformation between 1D and 2D co-ords") {
  SECTION("2x2 map") {
    auto data = R"(
      <terrain index="." id="grass" />
      <size x="2" y="2" />
      <row    y="0" terrain = ".." />
      <row    y="1" terrain = ".." />
    )";
    auto s = TestServer::WithDataString(data);

    CHECK(s->map().to1D(0, 0) == 0);
    CHECK(s->map().to1D(1, 0) == 1);
    CHECK(s->map().to1D(0, 1) == 2);
    CHECK(s->map().to1D(1, 1) == 3);
    CHECK(s->map().from1D(0) == std::make_pair<size_t, size_t>(0, 0));
    CHECK(s->map().from1D(1) == std::make_pair<size_t, size_t>(1, 0));
    CHECK(s->map().from1D(2) == std::make_pair<size_t, size_t>(0, 1));
    CHECK(s->map().from1D(3) == std::make_pair<size_t, size_t>(1, 1));
  }

  SECTION("3x3 map") {
    auto data = R"(
        <terrain index="." id="grass" />
        <size x="3" y="3" />
        <row    y="0" terrain = "..." />
        <row    y="1" terrain = "..." />
        <row    y="2" terrain = "..." />
      )";
    auto s = TestServer::WithDataString(data);

    CHECK(s->map().to1D(0, 0) == 0);
    CHECK(s->map().to1D(0, 1) == 3);
    CHECK(s->map().to1D(1, 1) == 4);
    CHECK(s->map().to1D(0, 2) == 6);
    CHECK(s->map().to1D(1, 2) == 7);
    CHECK(s->map().from1D(2) == std::make_pair<size_t, size_t>(2, 0));
    CHECK(s->map().from1D(3) == std::make_pair<size_t, size_t>(0, 1));
    CHECK(s->map().from1D(4) == std::make_pair<size_t, size_t>(1, 1));
    CHECK(s->map().from1D(6) == std::make_pair<size_t, size_t>(0, 2));
    CHECK(s->map().from1D(7) == std::make_pair<size_t, size_t>(1, 2));
  }

  SECTION("4x4 map") {
    auto data = R"(
        <terrain index="." id="grass" />
        <size x="4" y="4" />
        <row    y="0" terrain = "...." />
        <row    y="1" terrain = "...." />
        <row    y="2" terrain = "...." />
        <row    y="3" terrain = "...." />
      )";
    auto s = TestServer::WithDataString(data);

    CHECK(s->map().from1D(0) == std::make_pair<size_t, size_t>(0, 0));
    CHECK(s->map().from1D(6) == std::make_pair<size_t, size_t>(2, 1));
    CHECK(s->map().from1D(8) == std::make_pair<size_t, size_t>(0, 2));
    CHECK(s->map().from1D(9) == std::make_pair<size_t, size_t>(1, 2));
  }
}
