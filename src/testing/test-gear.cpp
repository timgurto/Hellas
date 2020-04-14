#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Damage is updated when a weapon depletes") {
  GIVEN("a consumable weapon that deals 100 damage") {
    auto data = R"(
      <item id="rock" gearSlot="6" >
        <weapon consumes="rock" damage="100" speed="1" range="100" />
      </item>
      <npcType id="ant" maxHealth="1" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto rock = &s.getFirstItem();
    user.gear(Item::WEAPON_SLOT).first = {
        rock, ServerItem::Instance::ReportingInfo::UserGear(&user,
                                                            Item::WEAPON_SLOT)};
    user.gear(Item::WEAPON_SLOT).second = 1;
    user.updateStats();

    WHEN("the weapon is used") {
      s.addNPC("ant", {10, 15});
      auto ant = s.getFirstNPC().serial();
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(ant));

      THEN("the player no longer does 100 damage") {
        WAIT_UNTIL(user.stats().weaponDamage < 100);
      }
    }
  }
}

TEST_CASE("Level requirements") {
  GIVEN("gear that requires level 2") {
    auto data = R"(
      <item id="fancyHat" lvlReq="2" gearSlot="0" />
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("a level-1 user with the item") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      const auto &fancyHat = s.getFirstItem();
      user.giveItem(&fancyHat);

      WHEN("he tries to equip it") {
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

        THEN("he is not wearing it") {
          REPEAT_FOR_MS(100);
          CHECK(!user.gear(0).first.hasItem());
        }
      }
    }
  }
}
