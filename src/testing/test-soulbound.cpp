#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Soulbound items can't be dropped") {
  GIVEN("rings binds on pickup") {
    useData(R"(
      <item id="ring" bind="pickup" />
    )");

    WHEN("a user receives a ring") {
      auto &ring = server->getFirstItem();
      user->giveItem(&ring);

      AND_WHEN("he tries to drop it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("it is still in his inventory") {
          REPEAT_FOR_MS(100);
          CHECK(user->inventory(0).first.hasItem());
        }

        THEN("he receives a warning") {
          CHECK(client->waitForMessage(WARNING_ITEM_IS_BOUND));
        }
      }

      THEN("he knows it's soulbound") {
        const auto &clientItem = client->inventory().at(0).first;
        WAIT_UNTIL(clientItem.type());

        CHECK(clientItem.isSoulbound());
      }
    }
  }

  GIVEN("rings bind on equip") {
    useData(R"(
      <item id="ring" bind="equip" gearSlot="1" />
    )");

    WHEN("a user receives a ring") {
      auto &ring = server->getFirstItem();
      user->giveItem(&ring);

      AND_WHEN("he tries to drop it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("it is no longer equipped") {
          WAIT_UNTIL(!user->gear(1).first.hasItem());
        }
      }

      AND_WHEN("he equips it") {
        client->sendMessage(
            CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0, Serial::Gear(), 1));

        AND_WHEN("he tries to drop it") {
          client->sendMessage(CL_DROP, makeArgs(Serial::Gear(), 1));

          THEN("it is still equipped") {
            REPEAT_FOR_MS(100);
            CHECK(user->gear(1).first.hasItem());
          }

          THEN("he receives a warning") {
            CHECK(client->waitForMessage(WARNING_ITEM_IS_BOUND));
          }
        }

        AND_WHEN("he unequips it") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Gear(), 1,
                                                      Serial::Inventory(), 0));

          AND_WHEN("he tries to drop it") {
            client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

            THEN("it is still in his inventory") {
              REPEAT_FOR_MS(100);
              CHECK(user->inventory(0).first.hasItem());
            }
          }
        }
      }
    }
  }

  GIVEN("a user has a plain item") {
    useData(R"(
      <item id="rock" />
    )");
    user->giveItem(&server->getFirstItem());

    THEN("he knows it isn't soulbound") {
      const auto &clientItem = client->inventory().at(0).first;
      WAIT_UNTIL(clientItem.type());

      CHECK_FALSE(clientItem.isSoulbound());
    }
  }
}

// No trading
// Containers must be privately owned
// Container can't change hands
