#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Soulbound items can't be dropped") {
  GIVEN("rings binds on pickup") {
    useData(R"(
      <item id="ring" bind="pickup" />
    )");

    THEN("the user knows that rings bind on pickup") {
      CHECK(client->items().begin()->second.bindsOnPickup());
    }

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
}

TEST_CASE_METHOD(ServerAndClientWithData, "By default, items do not bind") {
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

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Soulbound items can be stored only in private containers") {
  GIVEN("apples are BoP") {
    useData(R"(
      <item id="apple" bind="pickup" />
      <objectType id="barrel" >
        <container slots="1" />
      </objectType>
    )");

    AND_GIVEN("a user has an apple") {
      user->giveItem(&server->getFirstItem());

      AND_GIVEN("a publicly owned barrel") {
        const auto &barrel = server->addObject("barrel", {15, 15});

        WHEN("he tries to put it into the barrel") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      barrel.serial(), 0));

          THEN("he still has it") {
            REPEAT_FOR_MS(100);
            CHECK(user->inventory(0).first.hasItem());
          }
        }
      }
    }
  }
}

// No trading
// Container can't change hands
// Use as construction material is fine
// Persistence
// Soulbound status in SV_GEAR
// Swap from gear to inventory: item that was in inventory becomes soulbound
