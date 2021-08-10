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
          WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

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
            WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

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
        WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

        THEN("he has 1 fire resist and 0 armour") {
          CHECK(user->stats().fireResist == ArmourClass{1});
          CHECK(user->stats().armor == ArmourClass{0});
        }
      }
    }
  }
}
