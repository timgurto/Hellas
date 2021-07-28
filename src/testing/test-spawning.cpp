#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Simple spawner", "[spawning]") {
  GIVEN("a small object spawner") {
    auto data = R"(
      <objectType id="tree" />
      <spawnPoint
        y="32" x="32"
        type="tree"
        quantity="1"
        radius="1600"
        respawnTime="300000" />
    )";

    WHEN("the server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("there is an object") { s.getFirstObject(); }
    }
  }
}

TEST_CASE("Cached terrain for spawning", "[spawning]") {
  GIVEN("a large map with one tile of water, and a cached fish spawner") {
    auto data = R"(
      <objectType id="fish" allowedTerrain="water" />
      <spawnPoint useCachedTerrain="1" y="1600" x="1600" type="fish" quantity="1" radius="1600" respawnTime="300000" />
      <terrain index="." id="grass" />
      <terrain index="w" id="water" />
      <list id="default" default="1" >
        <allow id="grass" />
      </list>
      <list id="water" >
        <allow id="water" />
      </list>
      <size x="100" y="100" />
      <row y= "0" terrain = "...................................................................................................." />
      <row y= "1" terrain = "...................................................................................................." />
      <row y= "2" terrain = "...................................................................................................." />
      <row y= "3" terrain = "...................................................................................................." />
      <row y= "4" terrain = "...................................................................................................." />
      <row y= "5" terrain = "...................................................................................................." />
      <row y= "6" terrain = "...................................................................................................." />
      <row y= "7" terrain = "...................................................................................................." />
      <row y= "8" terrain = "...................................................................................................." />
      <row y= "9" terrain = "...................................................................................................." />
      <row y="10" terrain = "...................................................................................................." />
      <row y="11" terrain = "...................................................................................................." />
      <row y="12" terrain = "...................................................................................................." />
      <row y="13" terrain = "...................................................................................................." />
      <row y="14" terrain = "...................................................................................................." />
      <row y="15" terrain = "...................................................................................................." />
      <row y="16" terrain = "...................................................................................................." />
      <row y="17" terrain = "...................................................................................................." />
      <row y="18" terrain = "...................................................................................................." />
      <row y="19" terrain = "...................................................................................................." />
      <row y="20" terrain = "...................................................................................................." />
      <row y="21" terrain = "...................................................................................................." />
      <row y="22" terrain = "...................................................................................................." />
      <row y="23" terrain = "...................................................................................................." />
      <row y="24" terrain = "...................................................................................................." />
      <row y="25" terrain = "...................................................................................................." />
      <row y="26" terrain = "...................................................................................................." />
      <row y="27" terrain = "...................................................................................................." />
      <row y="28" terrain = "...................................................................................................." />
      <row y="29" terrain = "...................................................................................................." />
      <row y="30" terrain = "...................................................................................................." />
      <row y="31" terrain = "...................................................................................................." />
      <row y="32" terrain = "...................................................................................................." />
      <row y="33" terrain = "...................................................................................................." />
      <row y="34" terrain = "...................................................................................................." />
      <row y="35" terrain = "...................................................................................................." />
      <row y="36" terrain = "...................................................................................................." />
      <row y="37" terrain = "...................................................................................................." />
      <row y="38" terrain = "...................................................................................................." />
      <row y="39" terrain = "...................................................................................................." />
      <row y="40" terrain = "...................................................................................................." />
      <row y="41" terrain = "...................................................................................................." />
      <row y="42" terrain = "...................................................................................................." />
      <row y="43" terrain = "...................................................................................................." />
      <row y="44" terrain = "...................................................................................................." />
      <row y="45" terrain = "...................................................................................................." />
      <row y="46" terrain = "...................................................................................................." />
      <row y="47" terrain = "...................................................................................................." />
      <row y="48" terrain = "...................................................................................................." />
      <row y="49" terrain = "...................................................................................................." />
      <row y="50" terrain = "w..................................................................................................." />
      <row y="51" terrain = "...................................................................................................." />
      <row y="52" terrain = "...................................................................................................." />
      <row y="53" terrain = "...................................................................................................." />
      <row y="54" terrain = "...................................................................................................." />
      <row y="55" terrain = "...................................................................................................." />
      <row y="56" terrain = "...................................................................................................." />
      <row y="57" terrain = "...................................................................................................." />
      <row y="58" terrain = "...................................................................................................." />
      <row y="59" terrain = "...................................................................................................." />
      <row y="60" terrain = "...................................................................................................." />
      <row y="61" terrain = "...................................................................................................." />
      <row y="62" terrain = "...................................................................................................." />
      <row y="63" terrain = "...................................................................................................." />
      <row y="64" terrain = "...................................................................................................." />
      <row y="65" terrain = "...................................................................................................." />
      <row y="66" terrain = "...................................................................................................." />
      <row y="67" terrain = "...................................................................................................." />
      <row y="68" terrain = "...................................................................................................." />
      <row y="69" terrain = "...................................................................................................." />
      <row y="70" terrain = "...................................................................................................." />
      <row y="71" terrain = "...................................................................................................." />
      <row y="72" terrain = "...................................................................................................." />
      <row y="73" terrain = "...................................................................................................." />
      <row y="74" terrain = "...................................................................................................." />
      <row y="75" terrain = "...................................................................................................." />
      <row y="76" terrain = "...................................................................................................." />
      <row y="77" terrain = "...................................................................................................." />
      <row y="78" terrain = "...................................................................................................." />
      <row y="79" terrain = "...................................................................................................." />
      <row y="80" terrain = "...................................................................................................." />
      <row y="81" terrain = "...................................................................................................." />
      <row y="82" terrain = "...................................................................................................." />
      <row y="83" terrain = "...................................................................................................." />
      <row y="84" terrain = "...................................................................................................." />
      <row y="85" terrain = "...................................................................................................." />
      <row y="86" terrain = "...................................................................................................." />
      <row y="87" terrain = "...................................................................................................." />
      <row y="88" terrain = "...................................................................................................." />
      <row y="89" terrain = "...................................................................................................." />
      <row y="90" terrain = "...................................................................................................." />
      <row y="91" terrain = "...................................................................................................." />
      <row y="92" terrain = "...................................................................................................." />
      <row y="93" terrain = "...................................................................................................." />
      <row y="94" terrain = "...................................................................................................." />
      <row y="95" terrain = "...................................................................................................." />
      <row y="96" terrain = "...................................................................................................." />
      <row y="97" terrain = "...................................................................................................." />
      <row y="98" terrain = "...................................................................................................." />
      <row y="99" terrain = "...................................................................................................." />
    )";

    WHEN("the server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("a fish has spawned") {
        s.getFirstObject();

        AND_WHEN("another is spawned") {
          auto &spawner = s.getFirstSpawner();
          spawner.spawn();

          THEN("the two objects have different locations") {
            auto entities = s->findEntitiesInArea({1600, 1600}, 1600);
            CHECK(entities.size() == 2);
            auto it = entities.begin();
            const auto *fish1 = *it;
            ++it;
            const auto *fish2 = *it;

            CHECK(fish1->location() != fish2->location());
          }
        }
      }
    }
  }

  GIVEN("a cached spawner on a small map") {
    auto data = R"(
      <objectType id="mushroom" />
      <spawnPoint useCachedTerrain="1" y="100" x="100" type="mushroom"
  quantity="1" radius="100" respawnTime="300000" />
    )";

    WHEN("the server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("an object has spawned") { s.getFirstObject(); }
    }
  }

  SECTION("Distribution over multiple tiles", "[spawning]") {
    // GIVEN a cached spawner on a two-tile map
    auto data = R"(
      <objectType id="flower" />
      <spawnPoint useCachedTerrain="1" y="0" x="0" type="flower" quantity="0" radius="1000" respawnTime="300000" />
      <size x="1" y="2" />
      <row y= "0" terrain = "G" />
      <row y= "1" terrain = "G" />
    )";
    auto s = TestServer::WithDataString(data);

    // WHEN many objects are spawned
    auto firstTileOccupied = false;
    auto secondTileOccupied = false;
    for (auto i = 0; i != 20; ++i) {
      const auto &flower = s.getFirstSpawner().spawn();

      // THEN at the second and third tiles get objects
      auto row = s->map().getRow(flower->location().y);
      if (row == 0) firstTileOccupied = true;
      if (row == 1) secondTileOccupied = true;
      if (firstTileOccupied && secondTileOccupied) break;
    }
    CHECK(firstTileOccupied);
    CHECK(secondTileOccupied);
  }

  GIVEN("a small cached spawner on a large map") {
    auto data = R"(
      <objectType id="needle" />
      <spawnPoint useCachedTerrain="1" y="100" x="100" type="needle" quantity="1" radius="10" respawnTime="300000" />
      <size x="100" y="20" />
      <row y= "0" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "1" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "2" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "3" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "4" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "5" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "6" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "7" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "8" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y= "9" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="10" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="11" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="12" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="13" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="14" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="15" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="16" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="17" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="18" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row y="19" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
    )";

    WHEN("the server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("the object is inside the bounds of the spawner") {
        const auto &needle = s.getFirstObject();
        const auto SPAWN_CENTRE = MapPoint{100, 100};
        const auto RADIUS = 10;
        CHECK(distance(needle.location(), SPAWN_CENTRE) <= RADIUS);
      }
    }
  }
}

TEST_CASE("Dead objects respawn", "[spawning]") {
  GIVEN("an object that respawns instantly") {
    auto data = R"(
      <objectType id="whackamole" />
      <spawnPoint y="10" x="10" type="whackamole" quantity="1" radius="10" respawnTime="0" />
    )";

    auto s = TestServer::WithDataString(data);
    auto &mole = s.getFirstObject();

    WHEN("it dies") {
      mole.kill();

      THEN("there is another one") { WAIT_UNTIL(s.entities().size() == 2); }
    }
  }
}
