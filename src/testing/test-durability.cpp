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

      THEN("the tool is damaged") { CHECK(hat.health() < Item::MAX_HEALTH); }
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

// TODO:

// Shield takes damage on block, not hit
// Armour doesn't take damage on block
// Weapon/armour don't take damage on miss/dodge
// Tool objects damaged by use

// Can't block if shield is broken
// Can't construct from broken item
// Can't cast from broken item
// Can't use broken item as material
// Merchant objects can't trade with damaged items

// Item health preserved when swapping
// Health of items in containers is persistent

// Ranged weapons that deplete themselves don't damage the stack
