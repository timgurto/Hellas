#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping", "[scrapping]") {
  GIVEN("wood that can be scrapped into a woodchip") {
    useData(R"(
      <item id="wood" class="wood" />
      <item id="woodchip" />
      <itemClass id="wood">
        <canBeScrapped item="woodchip" />
      </itemClass>
    )");

    AND_GIVEN("the user has wood") {
      const auto &wood = server->findItem("wood");
      user->giveItem(&wood);

      WHEN("he scraps it") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(0, 0));

        THEN("he has a wood chip") {
          const auto &woodchip = server->findItem("woodchip");
          WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
        }
      }
    }
  }
}

// TODO
// Gear
// Container
// Bell curve
// Scrap one of a stack?
// Check inventory space
// Later: unlock repair skill
