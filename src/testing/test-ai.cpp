#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("NPCs chain pull", "[ai]") {
  GIVEN("a user with a spear") {
    auto data = R"(
      <npcType id="bear" />
      <item id="spear" gearSlot="6">
        <weapon range="25" consumes="spear" damage="1" speed="1" />
      </item>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);

    auto &user = s.getFirstUser();
    auto spear = &s.getFirstItem();
    user.giveItem(spear);
    c.sendMessage(CL_SWAP_ITEMS,
                  makeArgs(Client::INVENTORY, 0, Client::GEAR, 6));
    WAIT_UNTIL(user.gear(6).first.type() == spear);

    WHEN("there are two bears close to each other but out of aggro range") {
      s.addNPC("bear", {100, 5});
      auto bear1Serial = s.getFirstNPC().serial();
      s.addNPC("bear", {100, 15});
      auto bear2Serial = bear1Serial + 1;

      AND_WHEN("There's a third bear off in the distance") {
        s.addNPC("bear", {10, 300});
        auto bear3Serial = bear2Serial + 1;

        NPC *bear1, *bear2, *bear3;
        for (auto ent : s.entities()) {
          if (ent->serial() == bear1Serial)
            bear1 = dynamic_cast<NPC *>(ent);
          else if (ent->serial() == bear2Serial)
            bear2 = dynamic_cast<NPC *>(ent);
          else if (ent->serial() == bear3Serial)
            bear3 = dynamic_cast<NPC *>(ent);
        }

        AND_WHEN("the user throws a spear at one") {
          WAIT_UNTIL(c.objects().size() >= 2);
          c.sendMessage(CL_TARGET_ENTITY, makeArgs(bear1Serial));

          THEN("the two close bears are aware of him") {
            WAIT_UNTIL(bear1->isAwareOf(user));
            WAIT_UNTIL(bear2->isAwareOf(user));

            AND_THEN("the distant bear is not") {
              CHECK_FALSE(bear3->isAwareOf(user));
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

TEST_CASE("NPCs can get around obstacles") {
  GIVEN("a wall between an NPC and a player") {
    WHEN("the NPC becomes aware of the user") {
      THEN("it can reach him") {}
    }
  }
}
