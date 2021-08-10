#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Suffixes add stats", "[suffixes]") {
  GIVEN("a sword with a +1-armour suffix") {
    useData(R"(
      <item id="sword" gearSlot="weapon" >
        <randomSuffix fromSet="suffixes" />
      </item>

      <suffixSet id="suffixes" >
        <suffix id="extraArmour" />
      </suffixSet>

      <suffix id="extraArmour">
        <stats armour="1" />
      </suffix>
    )");

    WHEN("the user gets a sword") {
      const auto &sword = server->getFirstItem();
      user->giveItem(&sword);

      AND_WHEN("he equips it") {
        client->sendMessage(
            CL_SWAP_ITEMS,
            makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));
        WAIT_UNTIL(user->gear()[Item::WEAPON].first.hasItem());

        THEN("he has 1 armour") {
          const auto expectedArmour = ArmourClass{1};
          CHECK(user->stats().armor == expectedArmour);
        }
      }
    }
  }
}
