#include <cassert>

#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

using ItemReportingInfo = ServerItem::Instance::ReportingInfo;

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Item health is derived from item level and quality",
                 "[damage-on-use]") {
  SECTION("item level") {
    GIVEN("copper is level-10 and bronze is level-20") {
      useData(R"(
        <item id="copper" ilvl="10" />
        <item id="bronze" ilvl="20" />
      )");

      WHEN("a user has one of each") {
        user->giveItem(&server->findItem("copper"));
        user->giveItem(&server->findItem("bronze"));

        THEN("the bronze has more health") {
          const auto &copperInInventory = user->inventory(0);
          const auto &bronzeInInventory = user->inventory(1);

          CHECK(copperInInventory.health() < bronzeInInventory.health());
        }
      }
    }
  }

  SECTION("item level") {
    GIVEN("lead is common and gold is rare (with equal significant ilvls)") {
      useData(R"(
        <item id="lead" quality="1" ilvl="60" />
        <item id="gold" quality="3" ilvl="60" />
      )");

      WHEN("a user has one of each") {
        user->giveItem(&server->findItem("lead"));
        user->giveItem(&server->findItem("gold"));

        THEN("the gold has more health") {
          const auto &leadInInventory = user->inventory(0);
          const auto &goldInInventory = user->inventory(1);

          CHECK(leadInInventory.health() < goldInInventory.health());
        }
      }
    }
  }
}

TEST_CASE("Newly given items have full health", "[damage-on-use]") {
  GIVEN("apple items") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    const auto &apple = s.getFirstItem();

    WHEN("a user is given one") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      user.giveItem(&apple);

      THEN("it has full health") { CHECK_FALSE(user.inventory(0).isDamaged()); }
    }
  }
}

TEST_CASE("Combat reduces weapon/armour health", "[damage-on-use][combat]") {
  GIVEN(
      "a very fast, low-damage weapon; a very fast, low-damage enemy; some "
      "armour") {
    auto data = R"(
      <item id="tuningFork" gearSlot="weapon" >
        <weapon damage="1"  speed="0.01" />
      </item>
      <item id="hat" gearSlot="head" />
      <item id="shoes" gearSlot="feet" />
      <npcType id="hummingbird" level="1" attack="1" attackTime="10" maxHealth="1000000000" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    auto &weaponSlot = user.gear(Item::WEAPON);
    auto &headSlot = user.gear(Item::HEAD);
    auto &feetSlot = user.gear(Item::FEET);
    const auto &tuningFork = s.findItem("tuningFork");
    const auto &hat = s.findItem("hat");
    const auto &shoes = s.findItem("shoes");

    s.addNPC("hummingbird", {10, 15});
    const auto &hummingbird = s.getFirstNPC();

    WHEN("a player has the weapon equipped") {
      weaponSlot = {&tuningFork,
                    ItemReportingInfo::UserGear(&user, Item::WEAPON), 1};
      user.updateStats();

      AND_WHEN("he attacks the enemy with the weapon for a while") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(hummingbird.serial()));

        THEN("the weapon's health is reduced") {
          // Should result in about 1000 hits.  Hopefully enough for durability
          // to kick in.
          WAIT_UNTIL_TIMEOUT(weaponSlot.isDamaged(), 10000);

          AND_THEN("the user knows it") {
            const auto &cWeaponSlot = c.gear()[Item::WEAPON];
            WAIT_UNTIL(cWeaponSlot.first.type());
            WAIT_UNTIL(cWeaponSlot.first.isDamaged());
          }
        }
      }

      AND_WHEN("he dies") {
        user.kill();

        THEN("the weapon's health is reduced") {
          CHECK(weaponSlot.isDamaged());
        }
      }
    }

    WHEN("a player has the weapon and armour equipped") {
      weaponSlot = {&tuningFork,
                    ItemReportingInfo::UserGear(&user, Item::WEAPON), 1};
      headSlot = {&hat, ItemReportingInfo::UserGear(&user, Item::HEAD), 1};
      feetSlot = {&shoes, ItemReportingInfo::UserGear(&user, Item::FEET), 1};

      AND_WHEN("the enemy attacks for a while") {
        // Should happen automatically

        THEN("all armour gets damaged") {
          WAIT_UNTIL_TIMEOUT(headSlot.isDamaged(), 10000);
          WAIT_UNTIL_TIMEOUT(feetSlot.isDamaged(), 10000);
        }
      }

      AND_WHEN("the enemy attacks once") {
        auto &hummingbird = s.getFirstNPC();
        auto stats = hummingbird.stats();
        stats.attackTime = 1000;
        stats.crit = 0;
        hummingbird.stats(stats);

        CHECK_FALSE(headSlot.isDamaged());
        CHECK_FALSE(feetSlot.isDamaged());
        auto healthBefore = user.health();

        REPEAT_FOR_MS(1000);

        CHECK(user.health() >= healthBefore - 1);

        THEN("a maximum of one piece of armour is damaged") {
          auto bothDamaged = headSlot.isDamaged() && feetSlot.isDamaged();
          CHECK_FALSE(bothDamaged);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Shields are damaged by blocks and not hits",
                 "[damage-on-use][combat]") {
  GIVEN("a user with a shield that always blocks all damage, and an enemy") {
    useData(R"(
      <item id="shield" gearSlot="offhand" >
        <stats block="10000" blockValue="10000" health="10000" />
        <tag name="shield" />
      </item>
      <npcType id="hummingbird" level="1" attack="1" attackTime="1" maxHealth="1000000000" />
    )");
    server->addNPC("hummingbird", {10, 15});

    auto &shieldSlot = user->gear(Item::OFFHAND);
    shieldSlot = {&server->getFirstItem(),
                  ItemReportingInfo::UserGear(user, Item::OFFHAND), 1};
    user->updateStats();

    WHEN("the enemy hits the player many times (mostly blocks, no hits)") {
      THEN("the shield is damaged") {
        auto &shieldSlot = user->gear(Item::OFFHAND);
        WAIT_UNTIL_TIMEOUT(shieldSlot.isDamaged(), 10000);
      }
    }
  }
TEST_CASE("Thrown weapons don't take damage from attacking",
          "[damage-on-use][combat]") {
  GIVEN("a whale, and harpoons that can be thrown or shot") {
    auto data = R"(
      <item id="harpoon" gearSlot="weapon" stackSize="1000000" >
        <weapon damage="1" speed="0.01" consumes="harpoon" />
      </item>
      <item id="harpoonGun" gearSlot="weapon" stackSize="1000000" >
        <weapon damage="1" speed="0.01" consumes="harpoon" />
      </item>
      <npcType id="whale" level="1" attack="1" attackTime="1000000" maxHealth="1000000000" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addNPC("whale", {10, 15});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto *harpoon = &s.findItem("harpoon");
    const auto *harpoonGun = &s.findItem("harpoonGun");
    user.giveItem(harpoon, 1000000);
    user.giveItem(harpoonGun);

    WHEN("the user equips harpoons") {
      c.sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                            Serial::Gear(), Item::WEAPON));
      WAIT_UNTIL(user.gear(Item::WEAPON).hasItem());

      AND_WHEN("he attacks the whale many times") {
        const auto &equippedWeapon = user.gear(Item::WEAPON);
        auto whaleSerial = s.getFirstNPC().serial();
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(whaleSerial));
        REPEAT_FOR_MS(5000) {
          if (equippedWeapon.isDamaged()) break;
        }

        THEN("his remaining harpoons are at full health") {
          CHECK_FALSE(equippedWeapon.isDamaged());
        }
      }
    }

    WHEN("the user equips a harpoon gun") {
      c.sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 1,
                                            Serial::Gear(), Item::WEAPON));
      WAIT_UNTIL(user.gear(Item::WEAPON).hasItem());

      AND_WHEN("he attacks the whale many times") {
        const auto &equippedWeapon = user.gear(Item::WEAPON);
        auto whaleSerial = s.getFirstNPC().serial();
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(whaleSerial));
        REPEAT_FOR_MS(5000) {
          if (equippedWeapon.isDamaged()) break;
        }

        THEN("his harpoon gun is damaged") {
          CHECK(equippedWeapon.isDamaged());
        }
      }
    }
  }
}

TEST_CASE("Item damage is limited to 1", "[damage-on-use]") {
  GIVEN("a user with an item") {
    auto data = R"(
      <item id="thing" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(&s.getFirstItem());

    WHEN("it is used") {
      auto &itemInInventory = user.inventory(0);
      itemInInventory.onUseAsTool();

      THEN("it has lost at most 1 health") {
        const auto healthMissing =
            itemInInventory.type()->maxHealth() - itemInInventory.health();
        CHECK(healthMissing <= 1);
      }
    }
  }
}

TEST_CASE("Item damage happens randomly", "[damage-on-use]") {
  GIVEN("a user with an item") {
    auto data = R"(
      <item id="thing" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(&s.getFirstItem());

    WHEN("it is used {max-health} times") {
      auto &itemInInventory = user.inventory(0);
      for (auto i = 0; i != itemInInventory.type()->maxHealth(); ++i)
        itemInInventory.onUseAsTool();

      THEN("it still has at least 1 health") {
        CHECK(itemInInventory.health() >= 1);
      }
    }
  }
}

TEST_CASE("Crafting tools lose durability", "[damage-on-use][crafting][tool]") {
  GIVEN("a user with a crafting tool") {
    auto data = R"(
      <item id="rabbit" stackSize="1000" />
      <item id="hat">
        <tag name="rabbitHouse" />
      </item>
      <recipe id="rabbit">
        <tool class="rabbitHouse" />
      </recipe>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(&s.findItem("hat"));
    auto &hat = user.inventory(0);

    WHEN("the recipe is crafted many times") {
      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_CRAFT, makeArgs("rabbit", 1));
        REPEAT_FOR_MS(20);
        if (hat.isDamaged()) break;
      }

      THEN("the tool is damaged") {
        CHECK(hat.isDamaged());

        AND_THEN("the client knows it's damaged") {
          auto &cItem = c.inventory()[0];
          WAIT_UNTIL(cItem.first.type());
          WAIT_UNTIL(cItem.first.isDamaged());
        }
      }
    }

    WHEN("the tool is broken") {
      for (auto i = 0; i != 100000; ++i) {
        hat.onUseAsTool();
        if (hat.isBroken()) break;
      }
      CHECK(hat.isBroken());

      AND_WHEN("the user tries to craft the recipe") {
        c.sendMessage(CL_CRAFT, makeArgs("rabbit", 1));

        THEN("he doesn't have any products") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user.inventory(1).hasItem());
        }
      }
    }
  }
}

TEST_CASE("Construction tools lose durability",
          "[damage-on-use][construction][tool]") {
  GIVEN("a user with a construction tool") {
    auto data = R"(
      <objectType id="hole" constructionReq="digging" />
      <item id="shovel">
        <tag name="digging" />
      </item>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(&s.findItem("shovel"));
    user.addConstruction("hole");

    WHEN("many objects are constructed") {
      const auto &shovel = user.inventory(0);

      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_CONSTRUCT, makeArgs("hole", 10, 15));
        WAIT_UNTIL(s.entities().size() == i);
        if (shovel.isDamaged()) break;
      }

      THEN("the tool is damaged") { CHECK(shovel.isDamaged()); }
    }
  }
}

TEST_CASE("Gathering tools lose durability",
          "[damage-on-use][gathering][tool]") {
  GIVEN("a user with a shovel, and a garden with many onions") {
    auto data = R"(
      <objectType id="onionPatch" gatherReq="digging">
        <yield id="onion" initialMean="1000" />
      </objectType>
      <item id="shovel">
        <tag name="digging" />
      </item>
      <item id="onion" stackSize="1000"/>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("onionPatch", {10, 15});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(&s.findItem("shovel"));

    WHEN("many onions are gathered") {
      const auto &shovel = user.inventory(0);

      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_GATHER, makeArgs(s.getFirstObject().serial()));
        REPEAT_FOR_MS(20);
        if (shovel.isDamaged()) break;
      }

      THEN("the shovel is damaged") { CHECK(shovel.isDamaged()); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Tools don't take damage until gathering finishes",
                 "[damage-on-use][gathering][tool]") {
  GIVEN("gathering from a tree requires an axe and takes 10s") {
    useData(R"(
      <objectType id="tree" gatherReq="axe" gatherTime="10000">
        <yield id="wood" />
      </objectType>
      <item id="axe">
        <tag name="axe" />
      </item>
      <item id="wood" />
    )");
    const auto &tree = server->addObject("tree", {10, 10});

    AND_GIVEN("the user has an axe") {
      const auto *axe = &server->findItem("axe");
      user->giveItem(axe);

      WHEN("the client starts gathering 1000 times (but doesn't finish)") {
        for (auto i = 0; i != 1000; ++i)
          client->sendMessage(CL_GATHER, makeArgs(tree.serial()));

        THEN("the axe is undamaged") {
          REPEAT_FOR_MS(100);
          const auto &usersAxe = user->inventory(0);
          CHECK(usersAxe.health() == axe->maxHealth());
        }
      }
    }
  }
}

TEST_CASE("Tool objects lose durability",
          "[damage-on-use][construction][tool]") {
  GIVEN("a user with a construction tool") {
    auto data = R"(
      <objectType id="hole" constructionReq="digging" />
      <objectType id="earthMover" >
        <tag name="digging" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    s.addObject("earthMover", {15, 15});
    user.addConstruction("hole");

    WHEN("many objects are constructed") {
      const auto &earthMover = s.getFirstObject();
      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_CONSTRUCT, makeArgs("hole", 10, 15));
        WAIT_UNTIL(s.entities().size() == i + 1);
        if (earthMover.isMissingHealth()) break;
      }

      THEN("the tool is damaged") { CHECK(earthMover.isMissingHealth()); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Swapping items preserves damage",
                 "[damage-on-use][inventory]") {
  GIVEN("a user with two items") {
    useData(R"(
      <item id="coin"/>
    )");
    user->giveItem(&server->getFirstItem(), 2);

    WHEN("one is damaged") {
      auto &slot0 = user->inventory(0);
      do {
        slot0.onUseAsTool();
      } while (slot0.isAtFullHealth());
      auto itemHealth = slot0.health();

      AND_WHEN("he swaps them") {
        client->sendMessage(CL_SWAP_ITEMS, makeArgs(0, 0, 0, 1));
        REPEAT_FOR_MS(100);

        THEN("it is still damaged in its new location") {
          auto &slot1 = user->inventory(1);
          CHECK(slot1.health() == itemHealth);
        }
      }
    }
  }
}

TEST_CASE("Persistence of item health: users' items",
          "[damage-on-use][persistence][inventory]") {
  // Given a server with an item type
  auto data = R"(
    <item id="shoe" gearSlot="feet" />
  )";
  auto s = TestServer::WithDataString(data);
  auto invHealth = Hitpoints{0}, gearHealth = Hitpoints{0};
  auto username = ""s;

  // And a player has one in his inventory and one equipped
  {
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    username = user.name();
    user.giveItem(&s.getFirstItem(), 2);
    c.sendMessage(CL_SWAP_ITEMS, makeArgs(0, 1, 1, 5));

    // When the inventory item is damaged
    auto &invSlot = user.inventory(0);
    do {
      invSlot.onUseAsTool();
    } while (invSlot.isAtFullHealth());
    invHealth = invSlot.health();
    REQUIRE(invHealth >= 1);

    // And when the equipped item is damaged even more)
    auto &gearSlot = user.gear(5);
    WAIT_UNTIL(gearSlot.hasItem());
    do {
      gearSlot.onUseAsTool();
    } while (gearSlot.health() >= invHealth);
    gearHealth = gearSlot.health();

    // And when he logs off and back on
  }
  auto c = TestClient::WithUsernameAndDataString(username, data);
  s.waitForUsers(1);
  auto &user = s.getFirstUser();

  // Then the items are still damaged to the same degree
  CHECK(user.inventory(0).health() == invHealth);
  CHECK(user.gear(5).health() == gearHealth);
}

TEST_CASE("Persistence of item health: objects' contents",
          "[damage-on-use][persistence][containers]") {
  // Given a server with an item type and a container type
  auto itemHealth = Hitpoints{0};

  auto data = R"(
    <item id="toy" />
    <objectType id="toybox" >
      <container slots="1" />
    </objectType>
  )";

  {
    auto s = TestServer::WithDataString(data);

    // And a container object containing an item
    s.addObject("toybox");
    auto &toybox = s.getFirstObject();
    auto *toy = &s.getFirstItem();
    toybox.container().addItems(toy);

    // When the item is damaged
    auto &containerSlot = toybox.container().at(0);
    do {
      containerSlot.onUseAsTool();
    } while (containerSlot.isAtFullHealth());
    itemHealth = containerSlot.health();

    // And when the server restarts
  }
  auto s = TestServer::WithDataStringAndKeepingOldData(data);

  // Then the item is still damaged to the same degree
  auto &toybox = s.getFirstObject();
  CHECK(toybox.container().at(0).health() == itemHealth);
}

#define BREAK_ITEM(ITEM)               \
  for (auto i = 0; i != 100000; ++i) { \
    (ITEM).onUseAsTool();              \
    if ((ITEM).isBroken()) break;      \
  }                                    \
  CHECK((ITEM).isBroken());

TEST_CASE("Broken weapons don't add attack", "[damage-on-use][stats][gear]") {
  GIVEN("a weapon that deals 42 damage") {
    auto data = R"(
      <item id="sword" gearSlot="weapon" class="weapon" >
        <weapon damage="42"  speed="1" />
      </item>
      <itemClass id="weapon">
        <canBeRepaired/>
      </itemClass>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a player has one equipped") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      const auto DEFAULT_DAMAGE = User::OBJECT_TYPE.baseStats().weaponDamage;

      auto &weaponSlot = user.gear(Item::WEAPON);
      weaponSlot = {&s.getFirstItem(),
                    ItemReportingInfo::UserGear(&user, Item::WEAPON), 1};

      user.updateStats();
      CHECK(user.stats().weaponDamage == 42);

      AND_WHEN("it is broken") {
        for (auto i = 0; i != 100000; ++i) {
          user.onAttack();
          if (weaponSlot.isBroken()) break;
        }
        CHECK(weaponSlot.isBroken());

        THEN("his attack is the baseline User attack") {
          CHECK(user.stats().weaponDamage == DEFAULT_DAMAGE);

          AND_WHEN("he repairs it") {
            c.sendMessage(CL_REPAIR_ITEM,
                          makeArgs(Serial::Gear(), Item::WEAPON));

            THEN("his attack is higher than the baseline again") {
              WAIT_UNTIL(user.stats().weaponDamage > DEFAULT_DAMAGE);
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Broken shields don't block", "[damage-on-use][stats][gear]") {
  GIVEN("a shield") {
    auto data = R"(
      <item id="shield" gearSlot="offhand" >
        <tag name="shield" />
      </item>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a player has one equipped") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      auto &offhand = user.gear(Item::OFFHAND);
      offhand = {&s.getFirstItem(),
                 ItemReportingInfo::UserGear(&user, Item::OFFHAND), 1};
      user.updateStats();
      CHECK(user.canBlock());

      AND_WHEN("it is broken") {
        auto &shield = user.gear(Item::OFFHAND);
        BREAK_ITEM(shield);

        THEN("he can't block") { CHECK_FALSE(user.canBlock()); }
      }
    }
  }
}

TEST_CASE("Broken items can't be placed as objects",
          "[damage-on-use][construction]") {
  GIVEN("a seed that constructs a tree") {
    auto data = R"(
      <item id="seed" constructs="tree" />
      <objectType id="tree" constructionTime="1" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a user has a seed") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.giveItem(&s.getFirstItem());

      AND_WHEN("it is broken") {
        auto &seed = user.inventory(0);
        BREAK_ITEM(seed);

        AND_WHEN("he tries to construct a tree from it") {
          c.sendMessage(CL_CONSTRUCT_FROM_ITEM, makeArgs(0, 10, 15));

          THEN("there are no objects") {
            REPEAT_FOR_MS(100);
            CHECK(s.entities().empty());
          }
        }
      }
    }
  }
}

TEST_CASE("Broken items can't cast spells", "[damage-on-use][spells]") {
  GIVEN("a poisoned apple that deals damage to the user") {
    auto data = R"(
      <item id="poisonedApple" castsSpellOnUse="poison" />
      <spell id="poison" >
        <targets self="1" />
        <function name="doDirectDamage" i1="20" />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a user has one") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.giveItem(&s.getFirstItem());

      AND_WHEN("it is broken") {
        auto &poisonedApple = user.inventory(0);
        BREAK_ITEM(poisonedApple);

        AND_WHEN("he tries to use it") {
          c.sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

          THEN("he is still at full health") {
            REPEAT_FOR_MS(100);
            CHECK(!user.isMissingHealth());
          }
        }
      }
    }
  }
}

TEST_CASE("Broken items can't be used as construction materials",
          "[damage-on-use][construction]") {
  GIVEN("a tuffet made out of a rock") {
    auto data = R"(
      <item id="rock" />
      <objectType id="tuffet" >
        <material id="rock" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a user has a rock") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.giveItem(&s.getFirstItem());

      AND_WHEN("he places a tuffet construction site") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("tuffet", 10, 15));

        AND_WHEN("his rock is broken") {
          auto &rock = user.inventory(0);
          BREAK_ITEM(rock);

          AND_WHEN("he tries to use it to build the tuffet") {
            WAIT_UNTIL(s.entities().size() == 1);
            const auto &tuffet = s.getFirstObject();
            c.sendMessage(CL_SWAP_ITEMS,
                          makeArgs(Serial::Inventory(), 0, tuffet.serial(), 0));

            THEN("the tuffet is not finished") {
              REPEAT_FOR_MS(100);
              CHECK(tuffet.isBeingBuilt());
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Broken items can't be traded",
                 "[damage-on-use][merchant]") {
  GIVEN("an apple cart with an apple, and a user with a coin") {
    useData(R"(
      <item id="coin" />
      <item id="apple" />
      <objectType id="appleCart" merchantSlots="1" >
        <container slots="1" />
      </objectType>
    )");
    const auto *coin = &server->findItem("coin");
    const auto *apple = &server->findItem("apple");
    auto &appleCart =
        server->addObject("appleCart", {10, 15}, "someOtherOwner");
    appleCart.merchantSlot(0) = {apple, 1, coin, 1};
    appleCart.container().addItems(apple);

    user->giveItem(coin);

    AND_WHEN("his coin is broken") {
      auto &priceSlot = user->inventory(0);
      BREAK_ITEM(priceSlot);

      AND_WHEN("he tries to buy an apple") {
        client->sendMessage(CL_TRADE, makeArgs(appleCart.serial(), 0));

        THEN("he still has the coin") {
          REPEAT_FOR_MS(100);
          CHECK(user->inventory(0).type() == coin);
        }

        THEN("he gets a warning") {
          CHECK(client->waitForMessage(WARNING_PRICE_IS_BROKEN));
        }
      }
    }

    AND_WHEN("the apple is broken") {
      auto &wareSlot = appleCart.container().at(0);
      BREAK_ITEM(wareSlot);

      AND_WHEN("he tries to buy an apple") {
        client->sendMessage(CL_TRADE, makeArgs(appleCart.serial(), 0));

        THEN("he gets a warning") {
          CHECK(client->waitForMessage(WARNING_WARE_IS_BROKEN));
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Repairing items",
                 "[damage-on-use][repair][gear][containers]") {
  GIVEN("a user, repairable hats, and hatstand objects") {
    useData(R"(
      <itemClass id="headwear">
        <canBeRepaired/>
      </itemClass>
      <item id="hat" class="headwear" gearSlot="head" />
      <objectType id="hatstand"> <container slots="1"/> </objectType>
    )");
    const auto *hat = &server->getFirstItem();

    GIVEN("he has two hats in his inventory") {
      user->giveItem(hat, 2);
      auto &hat0 = user->inventory(0);
      auto &hat1 = user->inventory(1);

      WHEN("the first is broken") {
        BREAK_ITEM(hat0);

        AND_WHEN("he repairs it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("it's no longer broken") {
            WAIT_UNTIL(!hat0.isBroken());

            AND_THEN("the client knows its current health") {
              const auto &clientSlot = client->inventory()[0].first;
              WAIT_UNTIL(clientSlot.health() == hat0.health());
            }
          }
        }
      }

      WHEN("both are broken") {
        BREAK_ITEM(hat0);
        BREAK_ITEM(hat1);
        AND_WHEN("he repairs it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Inventory(), 1));

          THEN("it's no longer broken") {
            WAIT_UNTIL(!hat1.isBroken());

            AND_THEN("the first is still broken") { CHECK(hat0.isBroken()); }
          }
        }
      }
    }

    GIVEN("he is wearing a hat") {
      user->giveItem(hat);
      client->sendMessage(CL_SWAP_ITEMS,
                          makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));
      auto &headSlot = user->gear(0);
      WAIT_UNTIL(headSlot.hasItem());

      WHEN("it is broken") {
        BREAK_ITEM(headSlot);

        AND_WHEN("he repairs it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Gear(), 0));

          THEN("it is no longer broken") { WAIT_UNTIL(!headSlot.isBroken()); }
        }
      }
    }

    GIVEN("he owns a hatstand with a hat") {
      server->addObject("hatstand", {10, 15}, user->name());
      auto &hatstand = server->getFirstObject();
      hatstand.container().addItems(hat);

      WHEN("he tries to repair an invalid slot") {
        client->sendMessage(CL_REPAIR_ITEM, makeArgs(hatstand.serial(), 99));

        THEN("the server doesn't crash") {
          REPEAT_FOR_MS(100);
          CHECK(true);
        }
      }

      WHEN("the hat is broken") {
        auto &slot = hatstand.container().at(0);
        BREAK_ITEM(slot);

        AND_WHEN("he repairs it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(hatstand.serial(), 0));

          THEN("it's no longer broken") { WAIT_UNTIL(!slot.isBroken()); }
        }
      }
    }

    GIVEN("an unowned hatstand with a hat") {
      server->addObject("hatstand", {10, 15}, "someoneElse");
      auto &hatstand = server->getFirstObject();
      hatstand.container().addItems(hat);

      WHEN("the hat is broken") {
        auto &slot = hatstand.container().at(0);
        BREAK_ITEM(slot);

        AND_WHEN("he tries to repair it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(hatstand.serial(), 0));

          THEN("it's still broken") {
            REPEAT_FOR_MS(100);
            CHECK(slot.isBroken());
          }
        }
      }
    }

    WHEN("he tries to repair an item in a nonexistent container") {
      client->sendMessage(CL_REPAIR_ITEM, makeArgs(5, 0));

      THEN("the server doesn't crash") {
        REPEAT_FOR_MS(100);
        CHECK(true);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Non-repairable item",
                 "[damage-on-use][repair]") {
  GIVEN("a user with a non-repairable feather") {
    useData(R"(
      <item id="feather" />
    )");
    const auto *feather = &server->getFirstItem();
    user->giveItem(feather);

    WHEN("it is broken") {
      auto &slot = user->inventory(0);
      BREAK_ITEM(slot);

      AND_WHEN("he tries to repair it") {
        client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("it is still broken") {
          REPEAT_FOR_MS(100);
          CHECK(slot.isBroken());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Item repair that consumes items",
                 "[damage-on-use][repair]") {
  GIVEN("food repairs a broken heart") {
    useData(R"(
      <itemClass id="organ" >
        <canBeRepaired cost="food" />
      </itemClass>
      <item id="heart" class="organ" />
      <item id="food" />
    )");

    WHEN("a user has a heart") {
      user->giveItem(&server->findItem("heart"));

      AND_WHEN("it is broken") {
        auto &heart = user->inventory(0);
        BREAK_ITEM(heart);

        AND_WHEN("he tries to repair it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("it is still broken") {
            REPEAT_FOR_MS(100);
            CHECK(heart.isBroken());
          }
        }

        AND_WHEN("he has food") {
          user->giveItem(&server->findItem("food"));

          AND_WHEN("he tries to repair the heart") {
            client->sendMessage(CL_REPAIR_ITEM,
                                makeArgs(Serial::Inventory(), 0));

            THEN("it is no longer broken") {
              WAIT_UNTIL(!heart.isBroken());

              AND_THEN("he no longer has food") {
                const auto &foodSlot = user->inventory(1);
                WAIT_UNTIL(!foodSlot.hasItem());
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Item repair that requires a tool",
                 "[damage-on-use][repair][tool]") {
  GIVEN("a soldering iron is needed to repair a circuit") {
    useData(R"(
      <itemClass id="highTech" >
        <canBeRepaired tool="soldering" />
      </itemClass>
      <item id="circuit" class="highTech" />
      <item id="solderingIron">
        <tag name="soldering" />
      </item>
    )");

    WHEN("a user has a circuit") {
      user->giveItem(&server->findItem("circuit"));

      AND_WHEN("it is broken") {
        auto &circuit = user->inventory(0);
        BREAK_ITEM(circuit);

        AND_WHEN("he tries to repair it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("it is still broken") {
            REPEAT_FOR_MS(100);
            CHECK(circuit.isBroken());
          }
        }

        AND_WHEN("he has a soldering iron") {
          user->giveItem(&server->findItem("solderingIron"));

          AND_WHEN("he tries to repair the circuit") {
            client->sendMessage(CL_REPAIR_ITEM,
                                makeArgs(Serial::Inventory(), 0));

            THEN("it is no longer broken") { WAIT_UNTIL(!circuit.isBroken()); }
          }
        }
      }
    }
  }

  GIVEN("repairing a watch requires tweezers and costs parts") {
    useData(R"(
      <itemClass id="mechanical">
        <canBeRepaired tool="tweezers" cost="parts" />
      </itemClass>
      <item id="watch" class="mechanical" />
      <item id="tweezers">
        <tag name="tweezers" />
      </item>
      <item id="parts" />
    )");

    WHEN("a user has a watch and parts, but no tweezers") {
      user->giveItem(&server->findItem("watch"));
      user->giveItem(&server->findItem("parts"));

      AND_WHEN("it is broken") {
        auto &watch = user->inventory(0);
        BREAK_ITEM(watch);

        AND_WHEN("he tries to repair it") {
          client->sendMessage(CL_REPAIR_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("he still has his parts") {
            REPEAT_FOR_MS(100);
            CHECK(user->inventory(1).hasItem());
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Object repair",
                 "[damage-on-use][repair]") {
  GIVEN("a repairable object") {
    useData(R"(
      <objectType id="wall" maxHealth="100">
        <canBeRepaired />
      </objectType>
    )");

    auto &wall = server->addObject("wall", {10, 15});

    WHEN("it is missing 1 health") {
      wall.reduceHealth(1);

      AND_WHEN("a user tries to repair it") {
        client->sendMessage(CL_REPAIR_OBJECT, makeArgs(wall.serial()));

        THEN("it is back at full health") {
          WAIT_UNTIL(!wall.isMissingHealth());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Non-repairable objects",
                 "[damage-on-use][repair]") {
  GIVEN("a non-repairable object") {
    useData(R"(
      <objectType id="window" maxHealth="100" />
    )");

    auto &window = server->addObject("window", {10, 15}, user->name());

    WHEN("it is missing 1 health") {
      window.reduceHealth(1);

      AND_WHEN("a user tries to repair it") {
        client->sendMessage(CL_REPAIR_OBJECT, makeArgs(window.serial()));

        THEN("it is still not at full health") {
          REPEAT_FOR_MS(100);
          CHECK(window.isMissingHealth());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Repairing a nonexistent object",
                 "[repair]") {
  WHEN("a user tries to repair a nonexistent entity") {
    client.sendMessage(CL_REPAIR_OBJECT, "50");

    THEN("the server doesn't crash") {
      REPEAT_FOR_MS(100);
      CHECK(true);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Object repair at a cost",
                 "[damage-on-use][repair]") {
  GIVEN("an object that can be repaired for a cost") {
    useData(R"(
      <item id="snow" />
      <objectType id="snowman" maxHealth="100">
        <canBeRepaired cost="snow" />
      </objectType>
    )");

    auto &snowman = server->addObject("snowman", {10, 15}, user->name());

    WHEN("it is missing 1 health") {
      snowman.reduceHealth(1);

      AND_WHEN("a user tries to repair it") {
        client->sendMessage(CL_REPAIR_OBJECT, makeArgs(snowman.serial()));

        THEN("it is still not at full health") {
          REPEAT_FOR_MS(100);
          CHECK(snowman.isMissingHealth());
        }
      }

      AND_WHEN("a user has the cost item") {
        user->giveItem(&server->getFirstItem());

        AND_WHEN("he tries to repair it") {
          client->sendMessage(CL_REPAIR_OBJECT, makeArgs(snowman.serial()));

          THEN("it is  at full health") {
            WAIT_UNTIL(!snowman.isMissingHealth());

            AND_THEN("he no longer has the item") {
              WAIT_UNTIL(!user->inventory(0).hasItem());
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Object repair requiring a tool",
                 "[damage-on-use][repair][tool]") {
  GIVEN("an object that can be repaired using a tool") {
    useData(R"(
      <item id="wrench">
        <tag name="wrench" />
      </item>
      <objectType id="machine" maxHealth="100" >
        <canBeRepaired tool="wrench" />
      </objectType>
    )");

    auto &machine = server->addObject("machine", {10, 15}, user->name());

    WHEN("it is missing 1 health") {
      machine.reduceHealth(1);

      AND_WHEN("a user tries to repair it") {
        client->sendMessage(CL_REPAIR_OBJECT, makeArgs(machine.serial()));

        THEN("it is still not at full health") {
          REPEAT_FOR_MS(100);
          CHECK(machine.isMissingHealth());
        }
      }

      AND_WHEN("a user has the tool") {
        user->giveItem(&server->findItem("wrench"));

        AND_WHEN("he tries to repair it") {
          client->sendMessage(CL_REPAIR_OBJECT, makeArgs(machine.serial()));

          THEN("it is  at full health") {
            WAIT_UNTIL(!machine.isMissingHealth());
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Thrown weapons aren't damaged on death",
                 "[damage-on-use][death][gear]") {
  GIVEN("a user has a throwable grenade") {
    useData(R"(
      <item id="grenade" gearSlot="weapon" >
          <weapon damage="1" speed="1" consumes="grenade" />
      </item>
    )");
    user->gear(6) = {&server->getFirstItem(),
                     ServerItem::Instance::ReportingInfo::UserGear(user, 6), 1};

    WHEN("he dies") {
      user->kill();

      THEN("his grenade is still at full health") {
        REPEAT_FOR_MS(100);
        CHECK(user->gear(6).isAtFullHealth());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Item damage on death scales with max item health",
                 "[damage-on-use][death]") {
  GIVEN("a user has a high-health hat and a medium-health weapon") {
    useData(R"(
      <item id="highLevelHat" gearSlot="head" ilvl="100" />
      <item id="lowLevelWeapon" gearSlot="weapon" ilvl="50" />
    )");
    const auto *highLevelHat = &server->findItem("highLevelHat");
    const auto *lowLevelWeapon = &server->findItem("lowLevelWeapon");
    user->gear(Item::WEAPON) = {
        lowLevelWeapon,
        ServerItem::Instance::ReportingInfo::UserGear(user, Item::WEAPON), 1};
    user->gear(Item::HEAD) = {
        highLevelHat,
        ServerItem::Instance::ReportingInfo::UserGear(user, Item::HEAD), 1};

    WHEN("he dies") {
      user->kill();

      THEN("his weapon has taken less damage than his hat") {
        WAIT_UNTIL(user->gear(Item::HEAD).health() < highLevelHat->maxHealth());

        const auto hatFullHealth = highLevelHat->maxHealth(),
                   weaponFullHealth = lowLevelWeapon->maxHealth(),
                   hatHealth = user->gear(Item::HEAD).health(),
                   weaponHealth = user->gear(Item::WEAPON).health();

        const auto missingHatHealth = hatFullHealth - hatHealth,
                   missingWeaponHealth = weaponFullHealth - weaponHealth;

        INFO("Hat health: "s + toString(hatHealth) + "/"s +
             toString(hatFullHealth));
        INFO("Weapon health: "s + toString(weaponHealth) + "/"s +
             toString(weaponFullHealth));

        CHECK(missingHatHealth > missingWeaponHealth);
      }
    }
  }
}
