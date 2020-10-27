#include "TestClient.h"
#include "TestFixtures.h"
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
      <recipe id="grass" time="200" >
        <tool class="grassPicking" />
      </recipe>

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
    )";
    struct GrassPickingTool {
      std::string id;
      std::string description;
      bool canPickGrassAtDoubleSpeed;
    };
    auto grassPickingTools = std::vector<GrassPickingTool>{
        {"tweezers", "a simple tool", false},
        {"mower", "a double-speed tool", true},
        {"goat", "a tool with double speed for a different tag", false}};

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    const auto &grass = s.findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(&grass);

    for (const auto &tool : grassPickingTools) {
      AND_GIVEN("a user has " + tool.description) {
        s.waitForUsers(1);
        auto &user = s.getFirstUser();
        const auto &item = s.findItem(tool.id);
        user.giveItem(&item);

        WHEN("he starts crafting the recipe") {
          c.sendMessage(CL_CRAFT, "grass");

          AND_WHEN("150ms elapses") {
            REPEAT_FOR_MS(150);

            THEN("the product has "s +
                 (tool.canPickGrassAtDoubleSpeed ? "" : "not ") +
                 "been crafted") {
              CHECK(user.hasItems(expectedProduct) ==
                    tool.canPickGrassAtDoubleSpeed);
            }
          }
        }
      }
    }
  }

  GIVEN(
      "a 200ms recipe requires a tool, and the terrain is a double-speed "
      "tool") {
    auto data = R"(
      <item id="grass" />
      <recipe id="grass" time="200" >
        <tool class="grassSource" />
      </recipe>

      <terrain index="G" id="grass">
        <tag name="grassSource" toolSpeed = "2" />
      </terrain>
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="1" y="1" />
      <row y="0" terrain = "G" />

    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    const auto &grass = s.findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(&grass);

    WHEN("a user starts crafting the recipe") {
      s.waitForUsers(1);
      c.sendMessage(CL_CRAFT, "grass");

      AND_WHEN("150ms elapses") {
        REPEAT_FOR_MS(150);

        THEN("the product has been crafted") {
          auto &user = s.getFirstUser();
          CHECK(user.hasItems(expectedProduct));
        }
      }
    }
  }
}

TEST_CASE("The fastest tool is used") {
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
        c.sendMessage(CL_CRAFT, "grass");

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
  user.checkAndDamageToolAndGetSpeed("fakeTool");

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
                      makeArgs(Serial::Inventory(), 0, box.serial(), 0));

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

TEST_CASE("Extra items returned from crafting") {
  GIVEN("a nuclear reaction that takes U235 and returns 2 U238") {
    auto data = R"(
      <item id="u235" />
      <item id="u238" />
      <item id="electricity" />
      <recipe id="electricity" >
        <material id="u235" />
        <byproduct id="u238" quantity="2" />
      </recipe>
    )";
    auto s = TestServer::WithDataString(data);
    const auto &u235 = s.findItem("u235");
    const auto &u238 = s.findItem("u238");
    const auto &electricity = s.findItem("electricity");

    AND_GIVEN("he knows the recipe") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.addRecipe("electricity");

      AND_GIVEN("he has U235") {
        user.giveItem(&u235);

        WHEN("he makes electricity") {
          c.sendMessage(CL_CRAFT, "electricity");

          THEN("the user has electricity and U238") {
            auto expectedInInventory = ItemSet{};
            expectedInInventory.add(&u238, 2);
            expectedInInventory.add(&electricity);

            WAIT_UNTIL(user.hasItems(expectedInInventory));
          }
        }
      }

      AND_GIVEN("his inventory is full of U235 with one slot left") {
        user.giveItem(&u235, User::INVENTORY_SIZE - 1);

        WHEN("he tries to make electricity") {
          c.sendMessage(CL_CRAFT, "electricity");

          THEN("the user has no electricity") {
            auto product = ItemSet{};
            product.add(&electricity);

            REPEAT_FOR_MS(100);
            CHECK_FALSE(user.hasItems(product));
          }
        }
      }
    }
  }

  GIVEN("making money also provides happiness") {
    auto data = R"(
      <item id="money" />
      <item id="happiness" />
      <recipe id="money" >
        <byproduct id="happiness" />
      </recipe>
    )";
    auto s = TestServer::WithDataString(data);
    const auto &happiness = s.findItem("happiness");

    AND_GIVEN("a user knows how to make money") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.addRecipe("money");

      WHEN("he does so") {
        c.sendMessage(CL_CRAFT, "money");

        THEN("he has happiness") {
          auto expectedInInventory = ItemSet{};
          expectedInInventory.add(&happiness);

          WAIT_UNTIL(user.hasItems(expectedInInventory));
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Nonsense recipes can't be added") {
  WHEN("the user tries to learn a recipe that doesnt exist") {
    user->addRecipe("fakeRecipe");

    THEN("he still knows no recipes") { CHECK(user->knownRecipes().empty()); }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Recipe items") {
  GIVEN("a recipe item that teaches how to bake a cake") {
    useData(R"(
      <item id="cake" />
      <recipe id="cake" />
      <item id="cakeRecipe" castsSpellOnUse="teachRecipe" spellStringArg="cake" />
    )");

    THEN("the user can't craft anything") {
      CHECK(user->knownRecipes().empty());
    }

    WHEN("the user has the recipe item") {
      auto &cakeRecipe = server->findItem("cakeRecipe");
      user->giveItem(&cakeRecipe);

      AND_WHEN("he uses it") {
        client->sendMessage(CL_CAST_ITEM, makeArgs(0));

        THEN("he can craft something") {
          WAIT_UNTIL(user->knownRecipes().size() == 1);
        }
      }
    }
  }
}
