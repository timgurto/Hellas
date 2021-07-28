#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("No erroneous transform messages on login", "[transformation]") {
  TestServer s = TestServer::WithData("basic_rock");
  s.addObject("rock", {20, 20});
  TestClient c = TestClient::WithData("basic_rock");
  s.waitForUsers(1);

  bool transformTimeReceived = c.waitForMessage(SV_TRANSFORM_TIME, 200);
  CHECK_FALSE(transformTimeReceived);
}

TEST_CASE("Basic transformation", "[transformation]") {
  GIVEN("a sapling that turns into a tree after 0.1s") {
    auto data = R"(
      <objectType id="sapling">
        <transform id="tree" time="100" />
      </objectType>
      <objectType id="tree" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &obj = s.addObject("sapling", {10, 10});

    WHEN("0.2s pass") {
      REPEAT_FOR_MS(200);

      THEN("it is a tree") { CHECK(obj.type()->id() == "tree"); }
    }
  }
}

TEST_CASE("NPC transformation persists", "[transformation][persistence]") {
  // Given a Bulbasaur that turns into an Ivysaur after 2s
  auto data = R"(
      <npcType id="bulbasaur">
        <transform id="ivysaur" time="2000" />
      </npcType>
      <npcType id="ivysaur" />
    )";
  {
    auto s = TestServer::WithDataString(data);
    auto &bulbasaur = s.addNPC("bulbasaur", {10, 10});
    bulbasaur.permissions.setPlayerOwner("Alice");

    // When 1.5s elapses
    REPEAT_FOR_MS(1500);

    // And when the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);

    // Then the sapling has <=1s to go
    auto &bulbasaur = s.getFirstNPC();
    CHECK(bulbasaur.transformation.transformTimer() <= 1000);
  }
}

TEST_CASE("Transforming to a constructible object",
          "[construction][transformation]") {
  GIVEN("liquid metal that transforms into a T-1000 requiring sunglasses") {
    auto data = R"(
      <objectType id="liquidMetal" >
        <transform id="t1000" time="100" />
      </objectType>
      <objectType id="t1000" constructionTime="0" >
        <material id="sunglasses" quantity="1" />
      </objectType>
      <item id="sunglasses" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &obj = s.addObject("liquidMetal", {10, 10});

    WHEN("it transforms") {
      REPEAT_FOR_MS(200);

      THEN("it doesn't have sunglasses") { CHECK(obj.isBeingBuilt()); }
    }
  }
  GIVEN(
      "a fire that goes out, turning into a constructed (but normally "
      "constructible) pile of sticks") {
    auto data = R"(
      <objectType id="fire" >
        <transform id="stickPile" time="100" skipConstruction="1" />
      </objectType>
      <objectType id="stickPile" constructionTime="0" >
        <material id="stick" quantity="10" />
      </objectType>
      <item id="stick" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &obj = s.addObject("fire", {10, 10});

    WHEN("it transforms") {
      REPEAT_FOR_MS(200);

      THEN("it already has sticks") { CHECK_FALSE(obj.isBeingBuilt()); }
    }
  }
}

TEST_CASE("NPC transformation", "[transformation]") {
  GIVEN("a caterpillar that turns into a butterfly after 0.1s") {
    auto data = R"(
      <npcType id="caterpillar" maxHealth="1" >
        <transform id="butterfly" time="100" />
      </npcType>
      <npcType id="butterfly" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &npc = s.addNPC("caterpillar", {10, 10});

    WHEN("0.2s pass") {
      REPEAT_FOR_MS(200);

      THEN("it is still a caterpillar") {
        CHECK(npc.type()->id() == "caterpillar");
      }
    }

    WHEN("the caterpillar belongs to a user") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      npc.permissions.setPlayerOwner(c->username());

      AND_WHEN("0.2s pass") {
        REPEAT_FOR_MS(200);

        THEN("it is a butterfly") { CHECK(npc.type()->id() == "butterfly"); }
      }
    }
  }
}

TEST_CASE("Transform on gather", "[transformation][gathering]") {
  GIVEN("a tree that transforms into a stump when gathered") {
    auto data = R"(
      <objectType id="tree" >
        <yield id="wood" />
        <transform id="stump" whenEmpty="1" />
      </objectType>
      <objectType id="stump" />
      <item id="wood" />
    )";
    auto s = TestServer::WithDataString(data);
    const auto &tree = s.addObject("tree", {10, 15});

    WHEN("a user gathers from it") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      c.sendMessage(CL_GATHER, makeArgs(tree.serial()));

      THEN("it becomes a stump") { WAIT_UNTIL(tree.type()->id() == "stump"); }
    }
  }

  GIVEN("a butterfly that transforms into a caterpillar when gathered") {
    auto data = R"(
      <npcType id="butterfly" >
        <yield id="wings" />
        <transform id="caterpillar" whenEmpty="1" />
      </npcType>
      <npcType id="caterpillar" />
      <item id="wings" />
    )";
    auto s = TestServer::WithDataString(data);
    const auto &butterfly = s.addNPC("butterfly", {10, 15});
    CHECK(s.entities().size() == 1);

    WHEN("a user gathers from it") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      c.sendMessage(CL_GATHER, makeArgs(butterfly.serial()));

      THEN("it becomes a caterpillar") {
        REPEAT_FOR_MS(100);
        CHECK(s.entities().size() == 1);
      }
    }
  }
}
