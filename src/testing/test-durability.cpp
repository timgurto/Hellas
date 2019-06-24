#include <cassert>

#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

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
      weaponSlot.first = {&tuningFork};
      weaponSlot.second = 1;

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
      weaponSlot.first = {&tuningFork};
      weaponSlot.second = 1;

      headSlot.first = {&hat};
      headSlot.second = 1;

      feetSlot.first = {&shoes};
      feetSlot.second = 1;

      AND_WHEN("the enemy attacks for a while") {
        auto &hummingbird = s.getFirstNPC();

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
    }
  }
}

// Shield
// Tool items
// Tool objects

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
      weaponSlot.first = {&s.getFirstItem()};
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

  // Armour
  // Shield
  // Tool items
  // Construction from item
  // Cast from item
  // Use as material
}

// Tool objects

// Preserved when swapping
// Saved in container XML

// Ranged weapons that deplete themselves don't damage the stack

// Max 1 health loss per hit
// Health loss is a chance only
