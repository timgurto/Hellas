#include "TestFixtures.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Objects destroyed when used as tools",
                 "[construction][tool]") {
  GIVEN("a rock that is destroyed when used to build an anvil") {
    useData(R"(
      <objectType id="anvil" constructionTime="0" constructionReq="rock">
        <material id="iron" />
      </objectType>
      <objectType id="rock" destroyIfUsedAsTool="1" >
        <tag name="rock" />
      </objectType>
      <item id="iron" />
    )");

    const auto &rock = server->addObject("rock", {10, 15});

    WHEN("the user builds an anvil") {
      client->sendMessage(CL_CONSTRUCT, makeArgs("anvil", 10, 5));

      THEN("the rock is dead") { WAIT_UNTIL(rock.isDead()); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "The fastest tool is used",
                 "[crafting][tool]") {
  GIVEN("a 200ms recipe, a 1x tool and a 2x tool") {
    useData(R"(
      <item id="grass" />
      <recipe id="grass" time="200" >
        <tool class="grassPicking" />
      </recipe>

      <item id="tweezers">
        <tag name="grassPicking" />
      </item>
      <item id="mower">
        <tag name="grassPicking" toolSpeed = "2" />
      </item>
    )");

    const auto &grass = server->findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(&grass);

    AND_GIVEN("the user has one of each tool") {
      user->giveItem(&server->findItem("tweezers"));
      user->giveItem(&server->findItem("mower"));

      WHEN("he starts crafting the recipe") {
        client->sendMessage(CL_CRAFT, makeArgs("grass", 1));

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("the item has been crafted (i.e., the faster tool was used)") {
            CHECK(user->hasItems(expectedProduct));
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "NPCs don't cause tool checks to crash", "[tool]") {
  GIVEN("an NPC next to the user") {
    useData(R"( <npcType id="wolf" maxHealth="1" /> )");
    server->addNPC("wolf", user->location() + MapPoint{0, 5});

    WHEN("the user does a tool check") {
      user->getToolSpeed("fakeTool");

      THEN("the server doesn't crash") {}
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "The client knows a user's tools",
                 "[tool]") {
  GIVEN("a 'sawing' hacksaw and a 'pounding' hammer") {
    useData(R"(
      <item id="hacksaw" >
        <tag name="sawing" />
      </item>
    )");

    THEN("the client has no tools") {
      REPEAT_FOR_MS(100);
      CHECK(client->currentTools().empty());
    }

    AND_GIVEN("the user has a hacksaw") {
      user->giveItem(&server->findItem("hacksaw"));

      THEN("the client has a sawing tool") {
        WAIT_UNTIL(client->currentTools().size() == 1);
        const auto toolID = *client->currentTools().begin();
        CHECK(toolID == "sawing");
      }
    }
  }
}
