#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "NPCs chain pull") {
  GIVEN("a user with a spear") {
    useData(R"(
      <npcType id="bear" />
      <npcType id="critter"  isNeutral="1" />
      <item id="spear" gearSlot="6">
        <weapon range="25" consumes="spear" damage="1" speed="1" />
      </item>
    )");

    auto spear = &server->getFirstItem();
    user->giveItem(spear);
    client->sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Inventory(), 0, Serial::Gear(), 6));
    WAIT_UNTIL(user->gear(6).first.type() == spear);

    WHEN(
        "there are two bears and a neutral critter, close to each other but "
        "out of aggro range") {
      auto &bear1 = server->addNPC("bear", {100, 5});
      auto &bear2 = server->addNPC("bear", {100, 15});
      auto &critter = server->addNPC("critter", {100, 25});

      THEN("the two close bears are unaware of him") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(bear1.isAwareOf(*user));
        CHECK_FALSE(bear2.isAwareOf(*user));
      }

      AND_WHEN("There's a third bear off in the distance") {
        auto &bear3 = server->addNPC("bear", {10, 300});

        AND_WHEN("the user throws a spear at one") {
          WAIT_UNTIL(client->objects().size() >= 2);
          client->sendMessage(CL_TARGET_ENTITY, makeArgs(bear1.serial()));

          THEN("the two close bears are aware of him") {
            WAIT_UNTIL(bear1.isAwareOf(*user));
            WAIT_UNTIL(bear2.isAwareOf(*user));

            AND_THEN("the distant bear and critter are not") {
              CHECK_FALSE(bear3.isAwareOf(*user));
              CHECK_FALSE(critter.isAwareOf(*user));
            }
          }
        }
      }
    }
  }
}

TEST_CASE("NPCs don't attack each other") {
  GIVEN("An aggressive wolf NPC type") {
    auto data = R"(
      <npcType id="wolf" maxHealth="10000" attack="2" speed="10" />
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("Two wolves") {
      s.addNPC("wolf", {10, 15});
      s.addNPC("wolf", {15, 10});

      WHEN("Enough time passes for a few hits") {
        REPEAT_FOR_MS(50);

        THEN("the first hasn't lost any health") {
          const auto &wolf1 = s.getFirstNPC();
          CHECK(wolf1.health() == wolf1.stats().maxHealth);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Pathfinding") {
  GIVEN("a colliding square wall object type, and a wolf NPC") {
    auto data = ""s;
    data = R"(
      <npcType id="wolf" maxHealth="10000" attack="1" speed="100" >
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </npcType>
      <objectType id="wall">
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </objectType>
    )";

    SECTION("Single static obstacle") {
      GIVEN("a wall between a wolf and a player") {
        useData(data.c_str());
        server->addObject("wall", {60, 10});
        auto &wolf = server->addNPC("wolf", {90, 10});

        WHEN("the wolf starts chasing the user") {
          wolf.makeAwareOf(*user);

          THEN("it can reach him") {
            WAIT_UNTIL_TIMEOUT(distance(wolf, *user) <= wolf.attackRange(),
                               10000);
          }
        }
      }
    }

    SECTION("Random obstacles") {
      GIVEN("three walls randomly between a wolf and the user") {
        data += R"(
            <spawnPoint y="55" x="55" type="wall" quantity="3" radius="50" />
          )";
        useData(data.c_str());

        auto &wolf = server->addNPC("wolf", {100, 100});

        WHEN("the wolf starts chasing the user") {
          wolf.makeAwareOf(*user);

          THEN("it can reach him") {
            WAIT_UNTIL_TIMEOUT(distance(wolf, *user) <= wolf.attackRange(),
                               10000);
          }
        }
      }
    }
  }

  // React to target moving
}
