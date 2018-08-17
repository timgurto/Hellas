#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("NPCs chain pull") {
  GIVEN("a user with a spear") {
    auto data = R"(
      <npcType id="bear" />
      <item id="spear" gearSlot="6">
        <weapon range="25" consumes="spear" />
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
    WAIT_UNTIL(user.gear()[6].first == spear);

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
          WAIT_UNTIL(c.objects().size() == 2);
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

TEST_CASE("NPCs can get around obstacles") {
  GIVEN("a wall between an NPC and a player") {
    WHEN("the NPC becomes aware of the user") {
      THEN("it can reach him") {}
    }
  }
}
