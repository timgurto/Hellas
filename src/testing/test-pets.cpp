#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Taming NPCs") {
  GIVEN("a cat") {
    auto data = R"(
      <npcType id="cat" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    s.addNPC("cat");
    const auto &cat = s.getFirstNPC();

    THEN("it has no owner") {
      CHECK(cat.owner().type == Permissions::Owner::NONE);

      WHEN("a user tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial()));

        THEN("it belongs to the user") {
          WAIT_UNTIL(cat.owner().type == Permissions::Owner::PLAYER);
          CHECK(cat.owner().name == c->username());
        }
      }

      WHEN("a user tries to tame it with too many arguments") {
        c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial(), 42));

        THEN("it has no owner") {
          REPEAT_FOR_MS(50);
          CHECK(cat.owner().type == Permissions::Owner::NONE);
        }
      }
    }
  }
}

TEST_CASE("Bad arguments to taming command") {
  GIVEN("a server and client") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    WHEN("the client tries to tame a nonexistent NPC") {
      c.sendMessage(CL_TAME_NPC, makeArgs(42));

      THEN("the client receives a warning") {
        CHECK(c.waitForMessage(WARNING_DOESNT_EXIST));
      }
    }
  }
}

TEST_CASE("Targeting") {
  GIVEN("a dog with high health and low damage") {
    auto data = R"(
      <npcType id="dog" maxHealth="1000" attack="1" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("dog", {10, 15});
    auto &dog = s.getFirstNPC();

    WHEN("it becomes owned by a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      const auto &user = s.getFirstUser();
      dog.permissions().setPlayerOwner(user.name());

      THEN("it isn't targeting its owner") {
        REPEAT_FOR_MS(50);
        CHECK(dog.target() != &user);
      }
    }
  }
}
