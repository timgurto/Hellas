#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("No erroneous transform messages on login", "") {
  TestServer s = TestServer::WithData("basic_rock");
  s.addObject("rock", {20, 20});
  TestClient c = TestClient::WithData("basic_rock");
  s.waitForUsers(1);

  bool transformTimeReceived = c.waitForMessage(SV_TRANSFORM_TIME, 200);
  CHECK_FALSE(transformTimeReceived);
}

TEST_CASE("Basic transformation") {
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

TEST_CASE("NPC transformation") {
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
