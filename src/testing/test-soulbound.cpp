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
    }
  }

  GIVEN("rings binds on equip") {
    useData(R"(
      <item id="ring" bind="equip" />
    )");

    WHEN("a user receives a ring") {
      auto &ring = server->getFirstItem();
      user->giveItem(&ring);

      AND_WHEN("he tries to drop it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("it is no longer in his inventory") {
          WAIT_UNTIL(!user->inventory(0).first.hasItem());
        }
      }
    }
  }
}

// No trading
// Containers must be privately owned
// Container can't change hands
