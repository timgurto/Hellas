#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Objects destroyed when used as tools", "[construction][tool]") {
  GIVEN("a rock that is destroyed when used to build an anvil") {
    auto data = R"(
      <objectType id="anvil" constructionTime="0" constructionReq="rock">
        <material id="iron" />
      </objectType>
      <objectType id="rock" destroyIfUsedAsTool="1" >
        <tag name="rock" />
      </objectType>
      <item id="iron" />
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("a rock") {
      const auto &rock = s.addObject("rock", {10, 15});

      AND_GIVEN("a user") {
        auto c = TestClient::WithDataString(data);
        s.waitForUsers(1);

        WHEN("he builds an anvil") {
          c.sendMessage(CL_CONSTRUCT, makeArgs("anvil", 10, 5));

          THEN("the rock is dead") { WAIT_UNTIL(rock.isDead()); }
        }
      }
    }
  }
}

TEST_CASE("The fastest tool is used", "[crafting][tool]") {
  GIVEN("a 200ms recipe, a 1x tool and a 2x tool") {
    auto data = R"(
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
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    const auto &grass = s.findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(&grass);

    AND_GIVEN("a user has one of each tool") {
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.giveItem(&s.findItem("tweezers"));
      user.giveItem(&s.findItem("mower"));

      WHEN("he starts crafting the recipe") {
        c.sendMessage(CL_CRAFT, makeArgs("grass", 1));

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN(
              "the product has been crafted (meaning the faster tool was "
              "used)") {
            CHECK(user.hasItems(expectedProduct));
          }
        }
      }
    }
  }
}

TEST_CASE("NPCs don't cause tool checks to crash", "[tool]") {
  // Given a server and client with an NPC type defined;
  auto s = TestServer::WithData("wolf");
  auto c = TestClient::WithData("wolf");
  s.waitForUsers(1);
  auto &user = s.getFirstUser();

  // And an NPC;
  s.addNPC("wolf", user.location() + MapPoint{0, 5});

  // When hasTool() is called
  user.getToolSpeed("fakeTool");

  // Then the server doesn't crash
}
