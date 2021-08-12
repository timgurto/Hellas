#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Suffixes add stats", "[suffixes]") {
  GIVEN("a +1-armour suffix") {
    auto data = ""s;
    data = R"(
      <suffixSet id="suffixes" >
        <suffix id="extraArmour">
          <stats armour="1" />
        </suffix>
      </suffixSet>
    )";

    GIVEN("a sword with the suffix but no base stats") {
      data += R"(
        <item id="sword" gearSlot="weapon" >
          <randomSuffix fromSet="suffixes" />
        </item>
      )";
      useData(data.c_str());

      WHEN("the user gets a sword") {
        const auto &sword = server->getFirstItem();
        user->giveItem(&sword);

        AND_WHEN("he equips it") {
          client->sendMessage(
              CL_SWAP_ITEMS,
              makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));
          WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

          THEN("he has 1 armour") {
            CHECK(user->stats().armor == ArmourClass{1});
          }
        }
      }
    }

    SECTION("suffix stats are added to item's base stats") {
      GIVEN("a sword with 1 armour and the suffix") {
        data += R"(
          <item id="sword" gearSlot="weapon" >
            <randomSuffix fromSet="suffixes" />
            <stats armour="1" />
          </item>
        )";
        useData(data.c_str());

        WHEN("the user gets a sword") {
          const auto &sword = server->getFirstItem();
          user->giveItem(&sword);

          AND_WHEN("he equips it") {
            client->sendMessage(
                CL_SWAP_ITEMS,
                makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));
            WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

            THEN("he has 2 armour") {
              CHECK(user->stats().armor == ArmourClass{2});
            }
          }
        }
      }
    }
  }

  GIVEN("a sword with a +1 fire-resist suffix") {
    useData(R"(
      <suffixSet id="suffixes" >
        <suffix id="extraFireResist">
          <stats fireResist="1" />
        </suffix>
      </suffixSet>
    
      <item id="sword" gearSlot="weapon" >
        <randomSuffix fromSet="suffixes" />
      </item>
    )");

    WHEN("the user gets a sword") {
      const auto &sword = server->getFirstItem();
      user->giveItem(&sword);

      AND_WHEN("he equips it") {
        client->sendMessage(
            CL_SWAP_ITEMS,
            makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));
        WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

        THEN("he has 1 fire resist and 0 armour") {
          CHECK(user->stats().fireResist == ArmourClass{1});
          CHECK(user->stats().armor == ArmourClass{0});
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "The correct item suffix is applied",
                 "[suffixes]") {
  GIVEN("a sword with a 1-hit suffix, and a wand with a 1-healing suffix") {
    useData(R"(
      <suffixSet id="swordSuffixes" >
        <suffix id="extraHit">
          <stats hit="1" />
        </suffix>
      </suffixSet>
      <item id="sword" gearSlot="weapon" >
        <randomSuffix fromSet="swordSuffixes" />
      </item>

      <suffixSet id="wandSuffixes" >
        <suffix id="extraHealing">
          <stats healing="1" />
        </suffix>
      </suffixSet>
      <item id="magicWand" gearSlot="weapon" >
        <randomSuffix fromSet="wandSuffixes" />
      </item>
    )");

    WHEN("a user equips a sword") {
      const auto &sword = server->findItem("sword");
      user->giveItem(&sword);
      client->sendMessage(
          CL_SWAP_ITEMS,
          makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));

      THEN("he has 1 hit") { WAIT_UNTIL(user->stats().hit == BasisPoints{1}); }
    }

    WHEN("a user equips a magic wand") {
      const auto &magicWand = server->findItem("magicWand");
      user->giveItem(&magicWand);
      client->sendMessage(
          CL_SWAP_ITEMS,
          makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));

      THEN("he has 1 healing") {
        WAIT_UNTIL(user->stats().healing == BasisPoints{1});
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Multiple suffixes in a suffix set",
                 "[suffixes]") {
  GIVEN("a shield can have either fire or water resistance") {
    useData(R"(
      <suffixSet id="shieldSuffixes" >
        <suffix id="extraFireResist">
          <stats fireResist="1" />
        </suffix>
        <suffix id="extraWaterResist">
          <stats waterResist="1" />
        </suffix>
      </suffixSet>
      <item id="shield" gearSlot="offhand" >
        <randomSuffix fromSet="shieldSuffixes" />
      </item>
    )");

    WHEN("the user receives a shield in each inventory slot") {
      const auto *shield = &server->getFirstItem();
      user->giveItem(shield, User::INVENTORY_SIZE);

      THEN(
          "at least one shield has fire resistance, and at least one has water "
          "resistance") {
        bool fireResistanceFound{false}, waterResistanceFound{false};
        for (auto i = 0; i != User::INVENTORY_SIZE; ++i) {
          const auto statsFromThisShield =
              user->inventory()[i].first.statsFromSuffix();
          if (statsFromThisShield.fireResist == ArmourClass{1})
            fireResistanceFound = true;
          else if (statsFromThisShield.waterResist == ArmourClass{1})
            waterResistanceFound = true;
        }

        CHECK(fireResistanceFound);
        CHECK(waterResistanceFound);
      }
    }
  }
}

TEST_CASE("Item suffixes persist when user is offline", "[suffixes]") {
  // Given Alice has a sword with a fire-resist suffix
  const auto data = R"(
    <suffixSet id="swordSuffixes" >
      <suffix id="extraFireResist">
        <stats fireResist="1" />
      </suffix>
    </suffixSet>
    <item id="sword" gearSlot="weapon" >
      <randomSuffix fromSet="swordSuffixes" />
    </item>
  )";
  auto server = TestServer::WithDataString(data);

  {
    auto client = TestClient::WithUsernameAndDataString("Alice", data);
    server.waitForUsers(1);
    auto &alice = server.getFirstUser();
    alice.giveItem(&server.getFirstItem());

    // When she logs off
  }

  // And when she logs back in
  auto client = TestClient::WithUsernameAndDataString("Alice", data);
  server.waitForUsers(1);
  auto &alice = server.getFirstUser();

  // Then her sword still has the suffix
  const auto statsFromSuffix = alice.inventory()[0].first.statsFromSuffix();
  CHECK(statsFromSuffix.fireResist == ArmourClass{1});
}

TEST_CASE("Items loaded from data have the correct suffix", "[suffixes]") {
  // Given shields have either a fire-resist or a water-resist suffix
  const auto data = R"(
    <suffixSet id="shieldSuffixes" >
      <suffix id="extraFireResist">
        <stats fireResist="1" />
      </suffix>
      <suffix id="extraWaterResist">
        <stats waterResist="1" />
      </suffix>
    </suffixSet>
    <item id="shield" gearSlot="offhand" >
      <randomSuffix fromSet="shieldSuffixes" />
    </item>
  )";
  auto server = TestServer::WithDataString(data);

  // And given Alice has an inventory full of fire-resist shields
  {
    auto client = TestClient::WithUsernameAndDataString("Alice", data);
    server.waitForUsers(1);
    auto &alice = server.getFirstUser();

    for (auto i = 0; i != User::INVENTORY_SIZE; ++i) {
      const auto attempts = 100;
      for (auto attempt = 0; attempt != attempts; ++attempt) {
        alice.giveItem(&server.getFirstItem());
        const auto statsFromSuffix =
            alice.inventory()[i].first.statsFromSuffix();
        if (statsFromSuffix.fireResist == ArmourClass{1}) break;
        alice.inventory()[i].first = {};
      }
    }

    // When she logs off
  }

  // And when she logs back in
  auto client = TestClient::WithUsernameAndDataString("Alice", data);
  server.waitForUsers(1);
  auto &alice = server.getFirstUser();

  // Then all of her shields have fire resist
  for (auto i = 0; i != User::INVENTORY_SIZE; ++i) {
    const auto statsFromSuffix = alice.inventory()[i].first.statsFromSuffix();
    CHECK(statsFromSuffix.fireResist == ArmourClass{1});
  }
}

/*
TODO:
Persistence (in container/gear/inventory)
Propagate to client
Generate item names
Merchant-object search?
*/
