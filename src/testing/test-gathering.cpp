#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Gather an item from an object", "[gathering]") {
  auto s = TestServer::WithData("basic_rock");
  auto c = TestClient::WithData("basic_rock");

  // Add a single rock
  s.addObject("rock", {10, 10});
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // Gather
  auto serial = c.objects().begin()->first;
  c.sendMessage(CL_GATHER, makeArgs(serial));
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.action() ==
             User::Action::GATHER);  // Wait for gathering to start
  WAIT_UNTIL(user.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  // Make sure user has item
  const auto &item = *s.items().begin();
  CHECK(user.inventory(0).type() == &item);

  // Make sure object no longer exists
  CHECK(s.entities().empty());
}

/*
One gather worth of 1 million units of iron
1000 gathers worth of single rocks
This is to test the new gathering algorithm, which should favor rocks rather
than iron. It will fail randomly about 1 every 1000 times
*/
TEST_CASE_METHOD(ServerAndClientWithData,
                 "Gather chance is by gathers, not quantity", "[gathering]") {
  GIVEN(
      "an object with one gather's worth of 1M iron, and 1000 gathers' worth "
      "of single rocks") {
    useData(R"(
      <objectType id="ironDeposit" canGather=1 gatherTime=100>
          <yield id="rock" initialMean=1000 initialSD=0 />
          <yield id="iron" initialMean=1000000 initialSD=0 gatherMean=1000000 gatherSD=0 />
      </objectType>
      <item id="rock" />
      <item id="iron" stackSize=1000000 />
  )");
    const auto &ironDeposit = server->addObject("ironDeposit", {10, 10});

    WHEN("the user gathers from it once") {
      client->sendMessage(CL_GATHER, makeArgs(ironDeposit.serial()));
      WAIT_UNTIL(user->action() ==
                 User::Action::GATHER);  // Wait for gathering to start
      WAIT_UNTIL(user->action() ==
                 User::Action::NO_ACTION);  // Wait for gathering to finish

      THEN("he received a rock, and not iron") {
        const auto *rock = (*server)->findItem("rock");
        CHECK(user->inventory(0).type() == rock);
      }
    }
  }
}

TEST_CASE("Minimum yields", "[gathering]") {
  auto s = TestServer::WithData("min_apples");
  for (auto entity : s.entities()) {
    const Object *obj = dynamic_cast<const Object *>(entity);
    CHECK(obj->gatherable.hasItems());
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Gathering from an NPC",
                 "[gathering]") {
  GIVEN("an NPC with a yield") {
    useData(R"(
      <item id="fur" />
      <npcType id="mouse" maxHealth="1">
        <yield id="fur" />
      </npcType>
    )");

    auto &mouse = server->addNPC("mouse", {10, 15});
    mouse.permissions.setPlayerOwner(user->name());
    WAIT_UNTIL(mouse.gatherable.hasItems());

    AND_WHEN("a player tries to gather from it") {
      client->sendMessage(CL_GATHER, makeArgs(mouse.serial()));

      THEN("he has an item") { WAIT_UNTIL(user->inventory(0).hasItem()); }
    }

    AND_WHEN("a player right-clicks it") {
      WAIT_UNTIL(client->objects().size() == 1);
      auto &cMouse = client->getFirstNPC();
      cMouse.onRightClick();

      THEN("he has an item") {
        auto &user = server->getFirstUser();
        WAIT_UNTIL(user.inventory(0).hasItem());
      }
    }
  }

  GIVEN("an NPC that can be gathered with a tool") {
    useData(R"(
      <item id="fur" />
      <npcType id="mouse" maxHealth="1" gatherReq="tweezers">
        <yield id="fur" />
      </npcType>
    )");
    auto &mouse = server->addNPC("mouse", {10, 15});
    mouse.permissions.setPlayerOwner(user->name());

    WHEN("a user tries to gather from it") {
      client->sendMessage(CL_GATHER, makeArgs(mouse.serial()));

      THEN("he doesn't have an item") {
        REPEAT_FOR_MS(100);
        CHECK(!user->inventory(0).hasItem());
      }
    }
  }

  GIVEN("an NPC that takes 0.5s to gather") {
    useData(R"(
      <item id="egg" />
      <npcType id="chicken" maxHealth="1" gatherTime="500">
        <yield id="egg" />
      </npcType>
    )");
    auto &chicken = server->addNPC("chicken", {10, 15});
    chicken.permissions.setPlayerOwner(user->name());

    WHEN("a user tries to gather from it") {
      client->sendMessage(CL_GATHER, makeArgs(chicken.serial()));

      THEN("he doesn't have an item") {
        REPEAT_FOR_MS(100);
        CHECK(!user->inventory(0).hasItem());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Dead objects can't be gathered from",
                 "[gathering][death]") {
  GIVEN("a tree that yields wood") {
    useData(R"(
      <item id="wood" />
      <objectType id="tree" >
        <yield id="wood" />
      </objectType>
    )");
    auto &tree = server->addObject("tree", {10, 10});

    WHEN("the tree dies") {
      tree.kill();

      AND_WHEN("the user tries to gather from it") {
        client->sendMessage(CL_GATHER, makeArgs(tree.serial()));

        THEN("he receives nothing") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user->inventory(0).hasItem());
        }
      }
    }
  }
}
