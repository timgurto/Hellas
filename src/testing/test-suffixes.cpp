#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

/*
<item id="13U_copperHat" name="Bloody Helmet" >
  <stats armor="393" /> // these stats + stats in suffix set
  <generateItems suffixSet="57" /> // Named because each bunch of stats adds up
to 57 points
</item>

<suffixSet id="57" >
  <suffix id="phoenix_57" >
  <suffix id="lion_57" >
</suffixSet>

<suffix id="phoenix_57" suffix=" of the Phoenix" >
  <stats courage="2" fireResist="14" />
</suffix>
*/

TEST_CASE_METHOD(ServerAndClientWithData, "Simple suffixes", "[suffixes]") {
  GIVEN("a base item with a suffix") {
    useData(R"(
      <item id="sword" >
        <randomSuffix fromSet="suffixes" />
      </item>

      <suffixSet id="suffixes" >
        <suffix id="extraArmour" />
      </suffixSet>

      <suffix id="extraArmour" />
    )");

    THEN("the item's ID includes the suffix ID") {
      auto &item = server->getFirstItem();
      CHECK(item.id() == "sword_extraArmour");
    }
  }
}
