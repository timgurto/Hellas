#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Simple gear equip", "[gear]") {
  GIVEN("a user with gear in his inventory") {
    auto data = R"(
      <item id="hat" gearSlot="head" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto &hat = s.getFirstItem();
    user.giveItem(&hat);

    WHEN("he tries to equip it") {
      c.sendMessage(CL_SWAP_ITEMS,
                    makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

      THEN("he has an item in that gear slot") {
        WAIT_UNTIL(user.gear(0).hasItem());

        AND_WHEN("he takes it off") {
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Gear(), 0, Serial::Inventory(), 0));

          THEN("the server survives") { s.nop(); }
        }
      }
    }
  }
}

TEST_CASE("Damage is updated when a weapon depletes", "[gear][stats]") {
  GIVEN("a consumable weapon that deals 100 damage") {
    auto data = R"(
      <item id="rock" gearSlot="weapon" >
        <weapon consumes="rock" damage="100" speed="1" range="100" />
      </item>
      <npcType id="ant" maxHealth="1" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto rock = &s.getFirstItem();
    user.gear(Item::WEAPON) = {
        rock,
        ServerItem::Instance::ReportingInfo::UserGear(&user, Item::WEAPON), 1};
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

TEST_CASE_METHOD(ServerAndClientWithData, "Level requirements",
                 "[gear][leveling]") {
  GIVEN("a fancy hat that requires level 2, and a plain hat with no req") {
    useData(R"(
      <item id="plainHat" gearSlot="head" />
      <item id="fancyHat" lvlReq="2" gearSlot="head" />
    )");
    const auto &fancyHat = server->findItem("fancyHat");

    AND_GIVEN("a level-1 user with a fancy hat") {
      user->giveItem(&fancyHat);

      WHEN("he tries to equip it") {
        client->sendMessage(
            CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

        THEN("he is not wearing it") {
          REPEAT_FOR_MS(100);
          CHECK(!user->gear(0).hasItem());
        }
      }

      AND_GIVEN("he's level 2") {
        user->levelUp();

        WHEN("he tries to equip it") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      Serial::Gear(), 0));

          THEN("he is wearing it") { WAIT_UNTIL(user->gear(0).hasItem()); }
        }
      }

      WHEN("he tries to move it to inventory slot 1") {
        client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                    Serial::Inventory(), 1));

        THEN("inventory slot 1 has an item") {
          WAIT_UNTIL(user->inventory(1).hasItem());
        }
      }

      AND_GIVEN("he is already wearing a plain hat") {
        const auto &plainHat = server->findItem("plainHat");
        user->gear(0) = {&plainHat,
                         ServerItem::Instance::ReportingInfo::UserGear(user, 0),
                         1};

        WHEN("he tries to swap it for the fancy hat") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Gear(), 0,
                                                      Serial::Inventory(), 0));

          THEN("he is still wearing the plain hat") {
            REPEAT_FOR_MS(100);
            CHECK(user->gear(0).type()->id() == "plainHat");
          }
        }

        AND_GIVEN("he is level 2") {
          user->levelUp();

          WHEN("he tries to swap it for the fancy hat") {
            client->sendMessage(
                CL_SWAP_ITEMS,
                makeArgs(Serial::Gear(), 0, Serial::Inventory(), 0));

            THEN("he is wearing the fancy hat") {
              WAIT_UNTIL(user->gear(0).type()->id() == "fancyHat");
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Level requirements enforced on already-equipped gear",
          "[gear][leveling]") {
  GIVEN("a level-2 hat that gives 1000 health") {
    auto data = R"(
      <item id="hat" lvlReq="2" gearSlot="head" >
        <stats health="1000" />
      </item>
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("a level-1 user wearing one") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      const auto &hat = s.findItem("hat");

      user.gear(0) = {
          &hat, ServerItem::Instance::ReportingInfo::UserGear(&user, 0), 1};
      user.updateStats();

      THEN("his health is less than 1000") {
        CHECK(user.stats().maxHealth < 1000);
      }
    }
  }
}
