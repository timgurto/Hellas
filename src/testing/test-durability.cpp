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

TEST_CASE("Attacking reduces weapon health") {
  GIVEN("a very fast, low-damage weapon, and an enemy") {
    auto data = R"(
      <item id="tuning fork" gearSlot="6" >
        <weapon damage="1"  speed="0.01" />
      </item>
      <npcType id="hummingbird" level="1" maxHealth="1000000000" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    s.addNPC("hummingbird", {10, 15});
    const auto &hummingbird = s.getFirstNPC();

    WHEN("a player attacks the enemy with the weapon for a while") {
      auto &weaponSlot = user.gear(Item::WEAPON_SLOT);
      weaponSlot.first = {&s.getFirstItem()};
      weaponSlot.second = 1;
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(hummingbird.serial()));

      const auto &weapon = weaponSlot.first;
      // Should result in about 1000 hits.  Hopefully enough for durability to
      // kick in.
      REPEAT_FOR_MS(10000) {
        if (weapon.health() < ServerItem::MAX_HEALTH) break;
      }

      THEN("the weapon's health is reduced") {
        CHECK(weapon.health() < ServerItem::MAX_HEALTH);
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
}
