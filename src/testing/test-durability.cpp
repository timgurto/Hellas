#include <cassert>

#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

using ItemReportingInfo = ServerItem::Instance::ReportingInfo;

TEST_CASE("Newly given items have full health") {
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

      THEN("it has full health") {
        CHECK(user.inventory(0).first.health() == ServerItem::MAX_HEALTH);
      }
    }
  }
}

TEST_CASE("Combat reduces weapon/armour health") {
  GIVEN(
      "a very fast, low-damage weapon; a very fast, low-damage enemy; some "
      "armour") {
    auto data = R"(
      <item id="tuningFork" gearSlot="6" >
        <weapon damage="1"  speed="0.01" />
      </item>
      <item id="hat" gearSlot="0" />
      <item id="shoes" gearSlot="5" />
      <npcType id="hummingbird" level="1" attack="1" attackTime="10" maxHealth="1000000000" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto HEAD_SLOT = 0;
    const auto FEET_SLOT = 5;
    auto &weaponSlot = user.gear(Item::WEAPON_SLOT);
    auto &headSlot = user.gear(HEAD_SLOT);
    auto &feetSlot = user.gear(FEET_SLOT);
    const auto &tuningFork = s.findItem("tuningFork");
    const auto &hat = s.findItem("hat");
    const auto &shoes = s.findItem("shoes");

    s.addNPC("hummingbird", {10, 15});
    const auto &hummingbird = s.getFirstNPC();

    WHEN("a player has the weapon equipped") {
      weaponSlot.first = {
          &tuningFork, ItemReportingInfo::UserGear(&user, Item::WEAPON_SLOT)};
      weaponSlot.second = 1;
      user.updateStats();

      AND_WHEN("he attacks the enemy with the weapon for a while") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(hummingbird.serial()));

        THEN("the weapon's health is reduced") {
          // Should result in about 1000 hits.  Hopefully enough for durability
          // to kick in.
          WAIT_UNTIL_TIMEOUT(weaponSlot.first.health() < ServerItem::MAX_HEALTH,
                             10000);
        }
      }
    }

    WHEN("a player has the weapon and armour equipped") {
      weaponSlot.first = {
          &tuningFork, ItemReportingInfo::UserGear(&user, Item::WEAPON_SLOT)};
      weaponSlot.second = 1;

      headSlot.first = {&hat, ItemReportingInfo::UserGear(&user, HEAD_SLOT)};
      headSlot.second = 1;

      feetSlot.first = {&shoes, ItemReportingInfo::UserGear(&user, FEET_SLOT)};
      feetSlot.second = 1;

      AND_WHEN("the enemy attacks for a while") {
        // Should happen automatically

        THEN("all armour gets damaged") {
          WAIT_UNTIL_TIMEOUT(headSlot.first.health() < ServerItem::MAX_HEALTH,
                             10000);
          WAIT_UNTIL_TIMEOUT(feetSlot.first.health() < ServerItem::MAX_HEALTH,
                             10000);
        }

        THEN("the weapon's health is not reduced") {
          // Note: this will kill/respawn the player repeatedly.
          REPEAT_FOR_MS(10000);
          CHECK(weaponSlot.first.health() == ServerItem::MAX_HEALTH);
        }
      }

      AND_WHEN("the enemy attacks once") {
        auto &hummingbird = s.getFirstNPC();
        auto stats = hummingbird.stats();
        stats.attackTime = 1000;
        stats.crit = 0;
        hummingbird.stats(stats);

        CHECK(headSlot.first.health() == ServerItem::MAX_HEALTH);
        CHECK(feetSlot.first.health() == ServerItem::MAX_HEALTH);
        auto healthBefore = user.health();

        REPEAT_FOR_MS(1000);

        CHECK(user.health() >= healthBefore - 1);

        THEN("a maximum of one piece of armour is damaged") {
          auto hatDamaged = headSlot.first.health() < ServerItem::MAX_HEALTH;
          auto shoesDamaged = feetSlot.first.health() < ServerItem::MAX_HEALTH;
          auto bothDamaged = hatDamaged && shoesDamaged;
          CHECK_FALSE(bothDamaged);
        }
      }
    }
  }
}

TEST_CASE("Thrown weapons don't take damage from attacking") {
  GIVEN("a whale, and harpoons that can be thrown or shot") {
    auto data = R"(
      <item id="harpoon" gearSlot="6" stackSize="1000000" >
        <weapon damage="1" speed="0.01" consumes="harpoon" />
      </item>
      <item id="harpoonGun" gearSlot="6" stackSize="1000000" >
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
      c.sendMessage(CL_SWAP_ITEMS, makeArgs(Server::INVENTORY, 0, Server::GEAR,
                                            Item::WEAPON_SLOT));
      WAIT_UNTIL(user.gear(Item::WEAPON_SLOT).first.hasItem());

      AND_WHEN("he attacks the whale many times") {
        const auto &equippedWeapon = user.gear(Item::WEAPON_SLOT).first;
        auto whaleSerial = s.getFirstNPC().serial();
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(whaleSerial));
        REPEAT_FOR_MS(5000) {
          if (equippedWeapon.health() != Item::MAX_HEALTH) break;
        }

        THEN("his remaining harpoons are at full health") {
          CHECK(equippedWeapon.health() == Item::MAX_HEALTH);
        }
      }
    }

    WHEN("the user equips a harpoon gun") {
      c.sendMessage(CL_SWAP_ITEMS, makeArgs(Server::INVENTORY, 1, Server::GEAR,
                                            Item::WEAPON_SLOT));
      WAIT_UNTIL(user.gear(Item::WEAPON_SLOT).first.hasItem());

      AND_WHEN("he attacks the whale many times") {
        const auto &equippedWeapon = user.gear(Item::WEAPON_SLOT).first;
        auto whaleSerial = s.getFirstNPC().serial();
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(whaleSerial));
        REPEAT_FOR_MS(5000) {
          if (equippedWeapon.health() != Item::MAX_HEALTH) break;
        }

        THEN("his harpoon gun is damaged") {
          CHECK(equippedWeapon.health() != Item::MAX_HEALTH);
        }
      }
    }
  }
}

TEST_CASE("Item damage is limited to 1") {
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
      auto &itemInInventory = user.inventory(0).first;
      itemInInventory.onUse();

      THEN("it has lost at most 1 health") {
        CHECK(itemInInventory.health() >= Item::MAX_HEALTH - 1);
      }
    }
  }
}

TEST_CASE("Item damage happens randomly") {
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
      auto &itemInInventory = user.inventory(0).first;
      for (auto i = 0; i != Item::MAX_HEALTH; ++i) itemInInventory.onUse();

      THEN("it still has at least 1 health") {
        CHECK(itemInInventory.health() >= 1);
      }
    }
  }
}

TEST_CASE("Crafting tools lose durability") {
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
    auto &hat = user.inventory(0).first;

    WHEN("the recipe is crafted many times") {
      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_CRAFT, "rabbit");
        REPEAT_FOR_MS(20);
        if (hat.health() < Item::MAX_HEALTH) break;
      }

      THEN("the tool is damaged") {
        CHECK(hat.health() < Item::MAX_HEALTH);

        AND_THEN("the client knows it's damaged") {
          auto &cItem = c.inventory()[0].first;
          WAIT_UNTIL(cItem.health() < Item::MAX_HEALTH);
        }
      }
    }

    WHEN("the tool is broken") {
      for (auto i = 0; i != 100000; ++i) {
        hat.onUse();
        if (hat.isBroken()) break;
      }
      CHECK(hat.isBroken());

      AND_WHEN("the user tries to craft the recipe") {
        c.sendMessage(CL_CRAFT, "rabbit");

        THEN("he doesn't have any products") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user.inventory(1).first.hasItem());
        }
      }
    }
  }
}

TEST_CASE("Construction tools lose durability") {
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
      const auto &shovel = user.inventory(0).first;

      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_CONSTRUCT, makeArgs("hole", 10, 15));
        REPEAT_FOR_MS(20);
        if (shovel.health() < Item::MAX_HEALTH) break;
      }

      THEN("the tool is damaged") { CHECK(shovel.health() < Item::MAX_HEALTH); }
    }
  }
}

TEST_CASE("Tool objects lose durability") {
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
      auto maxHealth = earthMover.type()->baseStats().maxHealth;
      for (auto i = 0; i != 200; ++i) {
        c.sendMessage(CL_CONSTRUCT, makeArgs("hole", 10, 15));
        REPEAT_FOR_MS(20);
        if (earthMover.health() < maxHealth) break;
      }

      THEN("the tool is damaged") { CHECK(earthMover.health() < maxHealth); }
    }
  }
}

TEST_CASE("Swapping items preserves damage") {
  GIVEN("a user with two items") {
    auto data = R"(
      <item id="coin"/>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(&s.getFirstItem(), 2);

    WHEN("one is damaged") {
      auto &slot0 = user.inventory(0).first;
      do {
        slot0.onUse();
      } while (slot0.health() == Item::MAX_HEALTH);
      auto itemHealth = slot0.health();

      AND_WHEN("he swaps them") {
        c.sendMessage(CL_SWAP_ITEMS, makeArgs(0, 0, 0, 1));
        REPEAT_FOR_MS(100);

        THEN("it is still damaged in its new location") {
          auto &slot1 = user.inventory(1).first;
          CHECK(slot1.health() == itemHealth);
        }
      }
    }
  }
}

TEST_CASE("Persistence of item health: users' items") {
  // Given a server with an item type
  auto data = R"(
    <item id="shoe" gearSlot="5" />
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
    auto &invSlot = user.inventory(0).first;
    do {
      invSlot.onUse();
    } while (invSlot.health() == Item::MAX_HEALTH);
    invHealth = invSlot.health();

    // And when the equipped item is damaged even more)
    auto &gearSlot = user.gear(5).first;
    WAIT_UNTIL(gearSlot.hasItem());
    do {
      gearSlot.onUse();
    } while (gearSlot.health() >= invHealth);
    gearHealth = gearSlot.health();

    // And when he logs off and back on
  }
  auto c = TestClient::WithUsernameAndDataString(username, data);
  s.waitForUsers(1);
  auto &user = s.getFirstUser();

  // Then the items are still damaged to the same degree
  CHECK(user.inventory(0).first.health() == invHealth);
  CHECK(user.gear(5).first.health() == gearHealth);
}

TEST_CASE("Persistence of item health: objects' contents") {
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
    auto &containerSlot = toybox.container().at(0).first;
    do {
      containerSlot.onUse();
    } while (containerSlot.health() == Item::MAX_HEALTH);
    itemHealth = containerSlot.health();

    // And when the server restarts
  }
  auto s = TestServer::WithDataStringAndKeepingOldData(data);

  // Then the item is still damaged to the same degree
  auto &toybox = s.getFirstObject();
  CHECK(toybox.container().at(0).first.health() == itemHealth);
}

#define BREAK_ITEM(ITEM)               \
  for (auto i = 0; i != 100000; ++i) { \
    (ITEM).onUse();                    \
    if ((ITEM).isBroken()) break;      \
  }                                    \
  CHECK((ITEM).isBroken());

TEST_CASE("Broken items don't work") {
  GIVEN("a weapon that deals 42 damage") {
    auto data = R"(
      <item id="sword" gearSlot="6" >
        <weapon damage="42"  speed="1" />
      </item>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a player has one equipped") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      const auto DEFAULT_DAMAGE = User::OBJECT_TYPE.baseStats().weaponDamage;

      auto &weaponSlot = user.gear(Item::WEAPON_SLOT);
      weaponSlot.first = {&s.getFirstItem(), ItemReportingInfo::UserGear(
                                                 &user, Item::WEAPON_SLOT)};
      weaponSlot.second = 1;

      user.updateStats();
      CHECK(user.stats().weaponDamage == 42);

      AND_WHEN("it is broken") {
        for (auto i = 0; i != 100000; ++i) {
          user.onAttack();
          if (weaponSlot.first.isBroken()) break;
        }
        CHECK(weaponSlot.first.isBroken());

        THEN("his attack is the baseline User attack") {
          CHECK(user.stats().weaponDamage == DEFAULT_DAMAGE);
        }
      }
    }
  }

  GIVEN("a shield") {
    auto data = R"(
      <item id="shield" gearSlot="7" >
        <tag name="shield" />
      </item>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a player has one equipped") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      auto &offhand = user.gear(Item::OFFHAND_SLOT);
      offhand.first = {&s.getFirstItem(),
                       ItemReportingInfo::UserGear(&user, Item::OFFHAND_SLOT)};
      offhand.second = 1;
      user.updateStats();
      CHECK(user.canBlock());

      AND_WHEN("it is broken") {
        auto &shield = user.gear(Item::OFFHAND_SLOT).first;
        BREAK_ITEM(shield);

        THEN("he can't block") { CHECK_FALSE(user.canBlock()); }
      }
    }
  }

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
        auto &seed = user.inventory(0).first;
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
        auto &poisonedApple = user.inventory(0).first;
        BREAK_ITEM(poisonedApple);

        AND_WHEN("he tries to use it") {
          c.sendMessage(CL_CAST_ITEM, "0");

          THEN("he is still at full health") {
            REPEAT_FOR_MS(100);
            CHECK(user.health() == user.stats().maxHealth);
          }
        }
      }
    }
  }

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
          auto &rock = user.inventory(0).first;
          BREAK_ITEM(rock);

          AND_WHEN("he tries to use it to build the tuffet") {
            WAIT_UNTIL(s.entities().size() == 1);
            const auto &tuffet = s.getFirstObject();
            c.sendMessage(CL_SWAP_ITEMS,
                          makeArgs(Server::INVENTORY, 0, tuffet.serial(), 0));

            THEN("the tuffet is not finished") {
              REPEAT_FOR_MS(100);
              CHECK(tuffet.isBeingBuilt());
            }
          }
        }
      }
    }
  }

  GIVEN("an apple cart with an apple, and a user with a coin") {
    auto data = R"(
      <item id="coin" />
      <item id="apple" />
      <objectType id="appleCart" merchantSlots="1" bottomlessMerchant="1" >
        <container slots="1" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    const auto *coin = &s.findItem("coin");
    const auto *apple = &s.findItem("apple");
    s.addObject("appleCart", {10, 15}, "someOtherOwner");
    auto &appleCart = s.getFirstObject();
    appleCart.merchantSlot(0) = {apple, 1, coin, 1};

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.giveItem(coin);

    AND_WHEN("his coin is broken") {
      auto &invSlot = user.inventory(0).first;
      BREAK_ITEM(invSlot);

      AND_WHEN("he tries to buy an apple") {
        c.sendMessage(CL_TRADE, makeArgs(appleCart.serial(), 0));
        REPEAT_FOR_MS(100);

        THEN("he still has the coin") {
          CHECK(user.inventory(0).first.type() == coin);
        }
      }
    }
  }
}
