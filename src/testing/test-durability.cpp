#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Items in inventory have full health") {
  GIVEN("an item type") {
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
        CHECK(user.inventory(0).first.health == ServerItem::MAX_HEALTH);
      }
    }
  }
}

TEST_CASE("Using items reduces health") {
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
        if (weapon.health < ServerItem::MAX_HEALTH) break;
      }

      THEN("the weapon's health is reduced") {
        CHECK(weapon.health < ServerItem::MAX_HEALTH);
      }
    }
  }
}
