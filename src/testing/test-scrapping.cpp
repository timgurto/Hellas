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
        <canBeScrapped result="woodchip" />
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

  SECTION("Different items") {
    GIVEN("a rock that can be scrapped into sand") {
      useData(R"(
      <item id="rock" class="rock" />
      <item id="sand" />
      <itemClass id="rock">
        <canBeScrapped result="sand" />
      </itemClass>
    )");

      AND_GIVEN("the user has a rock") {
        const auto &rock = server->findItem("rock");
        user->giveItem(&rock);

        WHEN("he scraps it") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(0, 0));

          THEN("he has sand") {
            const auto &sand = server->findItem("sand");
            WAIT_UNTIL(user->inventory()[0].first.type() == &sand);
          }
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
