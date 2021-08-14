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
          WAIT_UNTIL(user->gear()[Item::WEAPON].hasItem());

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
            WAIT_UNTIL(user->gear()[Item::WEAPON].hasItem());

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
        WAIT_UNTIL(user->gear()[Item::WEAPON].hasItem());

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
              user->inventory()[i].statsFromSuffix();
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
  const auto statsFromSuffix = alice.inventory()[0].statsFromSuffix();
  CHECK(statsFromSuffix.fireResist == ArmourClass{1});
}

TEST_CASE_METHOD(ServerAndClientWithData, "Swapping items preserves suffixes",
                 "[suffixes]") {
  GIVEN("the user has a sword with a fire-resist suffix") {
    useData(R"(
      <suffixSet id="swordSuffixes" >
        <suffix id="extraFireResist">
          <stats fireResist="1" />
        </suffix>
      </suffixSet>
      <item id="sword" gearSlot="weapon" >
        <randomSuffix fromSet="swordSuffixes" />
      </item>
    )");
    user->giveItem(&server->getFirstItem());

    WHEN("he moves it to a different inventory slot") {
      client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                  Serial::Inventory(), 1));
      const auto &invSlot1 = user->inventory()[1];
      WAIT_UNTIL(invSlot1.hasItem());

      THEN("it still has fire resistance") {
        CHECK(invSlot1.statsFromSuffix().fireResist == ArmourClass{1});
      }
    }
  }
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
  const auto *shield = &server.getFirstItem();

  SECTION("Inventory") {
    auto shieldStats = std::vector<StatsMod>{};
    // And given Alice has an inventory full of shields
    {
      auto client = TestClient::WithUsernameAndDataString("Alice", data);
      server.waitForUsers(1);
      auto &alice = server.getFirstUser();

      for (auto i = 0; i != User::INVENTORY_SIZE; ++i) {
        alice.giveItem(shield);
        const auto statsFromSuffix = alice.inventory()[i].statsFromSuffix();
        shieldStats.push_back(statsFromSuffix);
      }

      // When she logs off
    }

    // And when she logs back in
    auto client = TestClient::WithUsernameAndDataString("Alice", data);
    server.waitForUsers(1);
    auto &alice = server.getFirstUser();

    // Then each of her shields has the same stats as before
    for (auto i = 0; i != User::INVENTORY_SIZE; ++i) {
      const auto statsFromSuffix = alice.inventory()[i].statsFromSuffix();
      CHECK(statsFromSuffix.fireResist == shieldStats[i].fireResist);
    }
  }

  SECTION("Gear") {
    //  Multiple repetitions to ensure that both suffixes get generated
    const auto REPETITIONS = 20;  // 1 in 2^20 chance to have lower coverage
    for (auto i = 0; i != REPETITIONS; ++i) {
      auto statsBefore = StatsMod{};

      // And given Alice has a shield equipped
      {
        auto client = TestClient::WithUsernameAndDataString("Alice", data);
        server.waitForUsers(1);
        auto &alice = server.getFirstUser();

        alice.giveItem(shield);
        client.sendMessage(
            CL_SWAP_ITEMS,
            makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::OFFHAND));
        WAIT_UNTIL(alice.gear()[Item::OFFHAND].hasItem());

        statsBefore = alice.gear()[Item::OFFHAND].statsFromSuffix();

        // When she logs off
      }

      // And when she logs back in
      auto client = TestClient::WithUsernameAndDataString("Alice", data);
      server.waitForUsers(1);
      auto &alice = server.getFirstUser();

      // Then her shield has the same stats as before
      const auto statsAfter = alice.gear()[Item::OFFHAND].statsFromSuffix();
      REQUIRE(statsBefore.fireResist == statsAfter.fireResist);
      REQUIRE(statsBefore.waterResist == statsAfter.waterResist);
    }
  }
}

TEST_CASE("Suffixes on items in containers persist", "[suffixes]") {
  // Given a box containing a sword with a fire-resist suffix
  const auto data = R"(
    <suffixSet id="swordSuffixes" >
      <suffix id="extraFireResist">
        <stats fireResist="1" />
      </suffix>
    </suffixSet>
    <item id="sword" gearSlot="weapon" >
      <randomSuffix fromSet="swordSuffixes" />
    </item>
    <objectType id="box" >
      <container slots="1"/>
    </objectType>
  )";
  {
    auto server = TestServer::WithDataString(data);
    auto &box = server.addObject("box", {10, 10});
    box.container().addItems(&server.getFirstItem());

    // When the server restarts
  }
  {
    auto server = TestServer::WithDataStringAndKeepingOldData(data);

    // Then the sword still has the suffix
    const auto &box = server.getFirstObject();
    CHECK(box.container().at(0).suffix() == "extraFireResist"s);
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Suffix names", "[suffixes]") {
  GIVEN(
      "the user has \"Sword\" and \"Axe\" items with an \"of Power\" suffix") {
    useData(R"(
      <suffixSet id="weaponSuffixes" >
        <suffix id="power" name="Power" />
      </suffixSet>
      <item id="sword" name="Sword" gearSlot="weapon" >
        <randomSuffix fromSet="weaponSuffixes" />
      </item>
      <item id="axe" name="Axe" gearSlot="weapon" >
        <randomSuffix fromSet="weaponSuffixes" />
      </item>
    )");
    const auto *sword = &server->findItem("sword");
    const auto *axe = &server->findItem("axe");

    WHEN("the user has a sword") {
      user->giveItem(sword);

      THEN("the client knows its name is \"Sword of Power\"") {
        const auto &cSlot = client->inventory()[0].first;
        WAIT_UNTIL(cSlot.type() != nullptr);
        CHECK(cSlot.name() == "Sword of Power"s);
      }
    }

    WHEN("the user has an axe") {
      user->giveItem(axe);

      THEN("the client knows its name is \"Axe of Power\"") {
        const auto &cSlot = client->inventory()[0].first;
        WAIT_UNTIL(cSlot.type() != nullptr);
        CHECK(cSlot.name() == "Axe of Power"s);
      }
    }
  }

  SECTION("Distinct suffix sets") {
    GIVEN("\"Sword\"s have \"of Speed\", and \"Axe\"s have \"of Might\"") {
      useData(R"(
        <suffixSet id="swordSuffixes" >
          <suffix id="speed" name="Speed" />
        </suffixSet>
        <item id="sword" name="Sword" gearSlot="weapon" >
          <randomSuffix fromSet="swordSuffixes" />
        </item>

        <suffixSet id="axeSuffixes" >
          <suffix id="might" name="Might" />
        </suffixSet>
        <item id="axe" name="Axe" gearSlot="weapon" >
          <randomSuffix fromSet="axeSuffixes" />
        </item>
      )");

      WHEN("the user has a sword") {
        const auto *sword = &server->findItem("sword");
        user->giveItem(sword);

        THEN("the client knows its name is \"Sword of Speed\"") {
          const auto &cSlot = client->inventory()[0].first;
          WAIT_UNTIL(cSlot.type() != nullptr);
          CHECK(cSlot.name() == "Sword of Speed"s);
        }
      }

      WHEN("the user has an axe") {
        const auto *axe = &server->findItem("axe");
        user->giveItem(axe);

        THEN("the client knows its name is \"Axe of Might\"") {
          const auto &cSlot = client->inventory()[0].first;
          WAIT_UNTIL(cSlot.type() != nullptr);
          CHECK(cSlot.name() == "Axe of Might"s);
        }
      }
    }
  }

  SECTION("items with no suffix") {
    GIVEN("the user has a simple item named \"gold\"") {
      useData(R"(
        <item id="gold" name="Gold" />
      )");
      user->giveItem(&server->getFirstItem());

      THEN("the client knows its name is \"gold\"") {
        const auto &cSlot = client->inventory()[0].first;
        WAIT_UNTIL(cSlot.type() != nullptr);
        CHECK(cSlot.name() == "Gold"s);
      }
    }
  }
}
