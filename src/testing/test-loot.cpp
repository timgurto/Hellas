#include "../client/ui/TakeContainer.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Client gets loot info and can loot") {
  GIVEN("an NPC that always drops 1 gold") {
    auto data = R"(
      <npcType id="goldbug" maxHealth="1" >
        <loot id="gold" />
      </npcType>
      <item id="gold" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &goldbug = s.addNPC("goldbug", {10, 15});

    WHEN("a user kills it") {
      TestClient c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 1);
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(goldbug.serial()));
      WAIT_UNTIL(goldbug.isDead());

      THEN("the user can see one item in its loot window") {
        const auto &cGoldBug = c.getFirstNPC();
        WAIT_UNTIL(cGoldBug.lootContainer() != nullptr);
        WAIT_UNTIL(cGoldBug.lootContainer()->size() == 1);

        AND_THEN("the server survives a loot request") {
          c.sendMessage(CL_TAKE_ITEM, makeArgs(goldbug.serial(), 0));

          AND_THEN("the client receives the item") {
            WAIT_UNTIL(c.inventory()[0].first.type() != nullptr);
          }
        }
      }
    }
  }
}

TEST_CASE("Objects have health", "[strength]") {
  GIVEN("a chair type with strength 6*wood, and a wood item with strength 5") {
    auto data = R"(
      <objectType id="chair" name="Chair" >
        <durability item="wood" quantity="6" />
      </objectType>
      <item id="wood" name="Wood" durability="5" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a chair object is created") {
      s.addObject("chair");

      THEN("it has 30 (6*5) health") {
        CHECK(s.getFirstObject().health() == 30);
      }
    }
  }
}

TEST_CASE("Clients discern NPCs with no loot") {
  GIVEN("an ant with 1 health and no loot table") {
    auto data = R"(
      <npcType id="ant" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addNPC("ant");
    WAIT_UNTIL(c.objects().size() == 1);

    WHEN("the ant dies") {
      NPC &serverAnt = s.getFirstNPC();
      serverAnt.reduceHealth(1);

      THEN("the user doesn't believe he can loot it") {
        ClientNPC &clientAnt = c.getFirstNPC();
        REPEAT_FOR_MS(200);
        CHECK_FALSE(clientAnt.lootable());
      }
    }
  }
}

TEST_CASE("Chance for strength-items as loot from object", "[strength]") {
  GIVEN("a snowman made of 1000 1-health snowflake items") {
    auto data = R"(
      <item id="snowflake" stackSize="1000" durabilty="1" />
      <objectType id="snowman">
        <durability item="snowflake" quantity="1000" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    auto &snowman = s.addObject("snowman", {10, 15});

    WHEN("a user kills the snowman") {
      snowman.onAttackedBy(user, 1);
      snowman.reduceHealth(9999);

      THEN("the client finds out that it's lootable") {
        WAIT_UNTIL(c.objects().size() == 1);
        ClientObject &clientSnowman = c.getFirstObject();
        c.waitForMessage(SV_INVENTORY);

        WAIT_UNTIL(clientSnowman.lootable());

        AND_WHEN("he tries to take the item") {
          WAIT_UNTIL(clientSnowman.container().size() > 0);
          c.sendMessage(CL_TAKE_ITEM, makeArgs(snowman.serial(), 0));

          THEN("he recieves it") {
            WAIT_UNTIL(c.inventory()[0].first.type() != nullptr);
          }
        }
      }
    }
  }
}

TEST_CASE("Looting from a container", "[container][only]") {
  GIVEN("a chest that can store 1000 gold") {
    auto data = R"(
      <item id="gold" name="Gold" stackSize="100" />
      <objectType id="chest" name="Chest" >
        <container slots="10" />
      </objectType>
    )";
    TestServer s = TestServer::WithDataString(data);
    TestClient c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto &gold = s.getFirstItem();
    auto &chest = s.addObject("chest", {10, 15});

    AND_GIVEN("the chest is full of gold") {
      chest.container().addItems(&gold, 1000);

      WHEN("the user destroys the chest") {
        chest.onAttackedBy(user, 1);
        chest.reduceHealth(9999);
        REQUIRE_FALSE(chest.loot().empty());
        REQUIRE(c.waitForMessage(SV_INVENTORY));

        auto &clientChest = c.getFirstObject();
        WAIT_UNTIL(clientChest.lootable());

        AND_WHEN("he loots it") {
          for (size_t i = 0; i != 10; ++i)
            c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), i));

          THEN("he gets some gold") {
            WAIT_UNTIL(c.inventory()[0].first.type() != nullptr);

            AND_THEN("he doesn't get all of it") {
              WAIT_UNTIL(chest.container().isEmpty());
              ItemSet oneThousandGold;
              oneThousandGold.add(&gold, 1000);
              CHECK_FALSE(s.getFirstUser().hasItems(oneThousandGold));
            }
          }
        }
      }
    }

    WHEN("the user destroys the chest") {
      chest.onAttackedBy(user, 1);
      chest.reduceHealth(9999);

      THEN("there's no loot available") {
        REPEAT_FOR_MS(200);
        c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), 0));
        REPEAT_FOR_MS(200);
        CHECK(c.inventory()[0].first.type() == nullptr);
      }
    }

    SECTION("An owned container can be looted from") {
      AND_GIVEN("the chest has another owner") {
        chest.permissions.setPlayerOwner("Alice");

        AND_GIVEN("it contains gold") {
          chest.container().addItems(&gold, 100);

          WHEN("the user destroys the chest") {
            chest.onAttackedBy(user, 1);
            chest.reduceHealth(9999);
            REQUIRE_FALSE(chest.loot().empty());

            AND_WHEN("he loots it") {
              c.waitForMessage(SV_INVENTORY);
              c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), 0));

              THEN("he gets some gold") {
                WAIT_UNTIL(c.inventory()[0].first.type() != nullptr);
              }
            }
          }
        }
      }
    }
  }
}

// Note: currently expected to fail, since it doesn't take tagging into account.
TEST_CASE("New users are alerted to lootable objects", "[.flaky]") {
  // Given a running server;
  // And a snowflake item with 1 health;
  // And a snowman object type made of 1000 snowflakes;
  auto data = R"(
    <item id="snowflake" stackSize="1000" durabilty="1" />
    <objectType id="snowman">
      <durability item="snowflake" quantity="1000" />
    </objectType>
  )";
  TestServer s = TestServer::WithDataString(data);

  // And a snowman exists;
  s.addObject("snowman", {10, 15});

  // And the snowman is dead
  Object &snowman = s.getFirstObject();
  snowman.reduceHealth(9999);

  // When a client logs in
  TestClient c = TestClient::WithDataString(data);
  s.waitForUsers(1);

  // Then the client finds out that it's lootable
  CHECK(c.waitForMessage(SV_INVENTORY));
}

TEST_CASE("Non-taggers are not alerted to lootable objects") {
  // Given a running server;
  // And a snowflake item with 1 health;
  // And a snowman object type made of 1000 snowflakes;
  auto data = R"(
    <item id="snowflake" stackSize="1000" durabilty="1" />
    <objectType id="snowman">
      <durability item="snowflake" quantity="1000" />
    </objectType>
  )";
  TestServer s = TestServer::WithDataString(data);
  TestClient c = TestClient::WithDataString(data);
  s.waitForUsers(1);

  // And a snowman exists;
  s.addObject("snowman", {10, 15});

  // And the snowman dies without the user killing it
  Object &snowman = s.getFirstObject();
  snowman.reduceHealth(9999);

  // Then the client doesn't finds out that it's lootable
  CHECK_FALSE(c.waitForMessage(SV_INVENTORY));
}

TEST_CASE("Loot-table equality") {
  auto rock = ServerItem{"rock"};
  auto stick = ServerItem{"stick"};

  GIVEN("Two loot tables") {
    auto a = LootTable{}, b = LootTable{};
    a == b;

    WHEN("One has a rock") {
      a.addSimpleItem(&rock, 1.0);
      THEN("they are not equal") {
        CHECK(a != b);

        AND_WHEN("the other has a rock") {
          b.addSimpleItem(&rock, 1.0);
          THEN("they are equal") { CHECK(a == b); }
        }

        AND_WHEN("the other has a stick") {
          b.addSimpleItem(&stick, 1.0);
          THEN("they are not equal") { CHECK(a != b); }
        }
      }
    }

    WHEN("they have the same entries in different orders") {
      a.addSimpleItem(&rock, 1.0);
      a.addSimpleItem(&stick, 1.0);

      b.addSimpleItem(&stick, 1.0);
      b.addSimpleItem(&rock, 1.0);

      THEN("they are equal") { CHECK(a == b); }
    }

    WHEN("they differ only by loot chance") {
      a.addSimpleItem(&rock, 1.0);
      b.addSimpleItem(&rock, 0.5);

      THEN("they are not equal") { CHECK(a != b); }
    }

    WHEN("they differ only by distribution") {
      a.addNormalItem(&rock, 1.0);
      b.addNormalItem(&rock, 0.5);

      THEN("they are not equal") { CHECK(a != b); }
    }
  }
}

TEST_CASE("Composite loot tables") {
  GIVEN(
      "an NPC that drops X, and one that drops X via a standalone loot table") {
    auto data = R"(
      <item id="gold" />
      <lootTable id="wealthyMob" >
        <loot id="gold" />
      </lootTable>

      <npcType id="merchant" >
        <loot id="gold" />
      </npcType>
      <npcType id="trader" >
        <lootTable id="wealthyMob" />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    const auto &merchant =
        dynamic_cast<const NPCType &>(*s->findObjectTypeByID("merchant"));
    const auto &trader =
        dynamic_cast<const NPCType &>(*s->findObjectTypeByID("trader"));

    THEN("they have identical loot tables") {
      CHECK(merchant.lootTable() == trader.lootTable());
    }
  }
}

TEST_CASE("Clients know when loot is all gone") {
  GIVEN("an NPC that drops an item") {
    auto data = R"(
      <item id="wart" />
      <npcType id="frog" >
        <loot id="wart" />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    const auto &frog = s.addNPC("frog", {10, 15});

    WHEN("a user kills it") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(frog.serial()));
      WAIT_UNTIL(frog.isDead());

      AND_WHEN("he loots it") {
        c.sendMessage(CL_TAKE_ITEM, makeArgs(frog.serial(), 0));
        const auto &user = s.getFirstUser();
        WAIT_UNTIL(user.inventory(0).first.hasItem());

        THEN("he knows it isn't lootable") {
          const auto &cFrog = c.getFirstNPC();
          WAIT_UNTIL(!cFrog.lootable());
        }
      }
    }
  }
}
