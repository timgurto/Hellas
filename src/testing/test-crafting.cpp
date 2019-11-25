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

TEST_CASE("Tools can have speed modifiers") {
  GIVEN(
      "a 200ms recipe that requires a tool, and a variety of matching tools") {
    auto data = R"(
      <item id="grass" />
      <item id="tweezers">
        <tag name="grassPicking" />
      </item>
      <item id="mower">
        <tag name="grassPicking" toolSpeed = "2" />
      </item>
      <item id="goat">
        <tag name="grassPicking" />
        <tag name="bleating" toolSpeed = "2" />
      </item>
      <recipe id="grass" time="200" >
        <tool class="grassPicking" />
      </recipe>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto &grass = s.findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(&grass);

    AND_GIVEN("a user has a simple tool") {
      const auto &tweezers = s.findItem("tweezers");
      user.giveItem(&tweezers);

      WHEN("he crafts the recipe") {
        c.sendMessage(CL_CRAFT, "grass");

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("the product has not been crafted") {
            CHECK_FALSE(user.hasItems(expectedProduct));
          }
        }
      }
    }

    AND_GIVEN("a user has a double-speed tool") {
      const auto &mower = s.findItem("mower");
      user.giveItem(&mower);

      WHEN("he crafts the recipe") {
        c.sendMessage(CL_CRAFT, "grass");

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("the product has been crafted") {
            CHECK(user.hasItems(expectedProduct));
          }
        }
      }
    }

    AND_GIVEN("a user has a tool with an additional, double-speed tag") {
      const auto &mower = s.findItem("goat");
      user.giveItem(&mower);

      WHEN("he crafts the recipe") {
        c.sendMessage(CL_CRAFT, "grass");

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("the product has not been crafted") {
            CHECK_FALSE(user.hasItems(expectedProduct));
          }
        }
      }
    }
  }
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
  user.checkAndDamageTool("fakeTool");

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
    user.gear(5).first = {
        &sock, ServerItem::Instance::ReportingInfo::UserGear(&user, 5)};
    user.gear(5).second = 1;

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

TEST_CASE("Duping exploit") {
  GIVEN("a user with a container, and a recipe and its material") {
    auto data = R"(
      <item id="brick" />
      <item id="clay" />
      <recipe id="brick" time="100" >
        <material id="clay" />
      </recipe>
      <objectType id="box">
        <container slots="4" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &user = s.getFirstUser();
    user.addRecipe("brick");
    const auto *clay = &s.findItem("clay");
    user.giveItem(clay);

    s.addObject("box", {10, 15}, user.name());
    const auto &box = s.getFirstObject();

    WHEN("a user starts crafting the recipe") {
      c.sendMessage(CL_CRAFT, "brick");

      AND_WHEN("he puts the material into the container") {
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Server::INVENTORY, 0, box.serial(), 0));

        AND_WHEN("enough time passes for the crafting to finish") {
          REPEAT_FOR_MS(150);

          THEN("his inventory is still empty") {
            CHECK_FALSE(user.inventory(0).first.hasItem());
          }
        }
      }
    }
  }
}
