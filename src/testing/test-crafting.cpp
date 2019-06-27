#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Recipes can be known by default") {
  TestServer s = TestServer::WithData("box_from_nothing");
  TestClient c = TestClient::WithData("box_from_nothing");
  s.waitForUsers(1);

  User &user = s.getFirstUser();
  c.sendMessage(CL_CRAFT, "box");
  WAIT_UNTIL(user.action() ==
             User::Action::CRAFT);  // Wait for gathering to start
  WAIT_UNTIL(user.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  const ServerItem *itemInFirstSlot = user.inventory()[0].first.type();
  REQUIRE(itemInFirstSlot != nullptr);
  CHECK(itemInFirstSlot->id() == "box");
}

TEST_CASE("Terrain as tool", "[tool]") {
  TestServer s = TestServer::WithData("daisy_chain");
  TestClient c = TestClient::WithData("daisy_chain");
  s.waitForUsers(1);

  User &user = s.getFirstUser();
  c.sendMessage(CL_CRAFT, "daisyChain");
  WAIT_UNTIL(user.action() ==
             User::Action::CRAFT);  // Wait for gathering to start
  WAIT_UNTIL(user.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  const ServerItem *itemInFirstSlot = user.inventory()[0].first.type();
  REQUIRE(itemInFirstSlot != nullptr);
  CHECK(itemInFirstSlot->id() == "daisyChain");
}

TEST_CASE("Client sees default recipes") {
  TestServer s = TestServer::WithData("box_from_nothing");
  TestClient c = TestClient::WithData("box_from_nothing");
  s.waitForUsers(1);

  c.showCraftingWindow();

  CHECK(c.recipeList().size() == 1);
}

TEST_CASE("Crafting is allowed if materials will vacate a slot") {
  // Given a server and client;
  // And items/recipe for meat -> cooked meat;
  TestServer s = TestServer::WithData("cooking_meat");
  TestClient c = TestClient::WithData("cooking_meat");

  // And the user has an inventory full of meat
  s.waitForUsers(1);
  User &u = s.getFirstUser();
  const ServerItem &meat = *s.items().find(ServerItem("meat"));
  u.giveItem(&meat, User::INVENTORY_SIZE);

  // When he tries to craft cooked meat
  c.sendMessage(CL_CRAFT, "cookedMeat");
  WAIT_UNTIL(u.action() == User::Action::CRAFT);  // Wait for gathering to start
  WAIT_UNTIL(u.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  // Then his inventory contains cooked meat
  const ServerItem *itemInFirstSlot = u.inventory()[0].first.type();
  REQUIRE(itemInFirstSlot != nullptr);
  CHECK(itemInFirstSlot->id() == "cookedMeat");
}

TEST_CASE("NPCs don't cause tool checks to crash", "[tool][crash]") {
  // Given a server and client with an NPC type defined;
  auto s = TestServer::WithData("wolf");
  auto c = TestClient::WithData("wolf");
  s.waitForUsers(1);
  auto &user = s.getFirstUser();

  // And an NPC;
  s.addNPC("wolf", user.location() + MapPoint{0, 5});

  // When hasTool() is called
  user.findTool("fakeTool");

  // Then the server doesn't crash
}

TEST_CASE("Gear counts towards materials") {
  GIVEN("A user wearing an item, and a recipe that needs that item") {
    auto data = R"(
      <item id="sock" gearSlot="5" />
      <item id="sockPuppet" />
      <recipe id="sockPuppet" time="1" >
        <material id="sock" />
      </recipe>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    auto &sock = s.findItem("sock");
    user.gear(5) = std::make_pair(&sock, 1);

    WHEN("he tries to craft the recipe") {
      REPEAT_FOR_MS(100);
      c.sendMessage(CL_CRAFT, "sockPuppet");

      THEN("he has the new item") {
        auto &sockPuppet = s.findItem("sockPuppet");
        WAIT_UNTIL(user.inventory(0).first.type() == &sockPuppet);
      }
    }
  }
}
