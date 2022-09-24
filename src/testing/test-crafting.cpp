#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Recipes can be known by default", "[crafting]") {
  TestServer s = TestServer::WithData("box_from_nothing");
  TestClient c = TestClient::WithData("box_from_nothing");
  s.waitForUsers(1);

  User &user = s.getFirstUser();
  c.sendMessage(CL_CRAFT, makeArgs("box", 1));
  WAIT_UNTIL(user.action() ==
             User::Action::CRAFT);  // Wait for gathering to start
  WAIT_UNTIL(user.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  const ServerItem *itemInFirstSlot = user.inventory(0).type();
  REQUIRE(itemInFirstSlot != nullptr);
  CHECK(itemInFirstSlot->id() == "box");
}

TEST_CASE_METHOD(ServerAndClientWithData, "Tools can have speed modifiers",
                 "[crafting][tool]") {
  GIVEN(
      "a 200ms recipe that requires a tool, and a variety of matching tools") {
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
      <item id="goat">
        <tag name="grassPicking" />
        <tag name="bleating" toolSpeed = "2" />
      </item>
    )");
    struct GrassPickingTool {
      std::string id;
      std::string description;
      bool canPickGrassAtDoubleSpeed;
    };
    auto grassPickingTools = std::vector<GrassPickingTool>{
        {"tweezers", "a simple tool", false},
        {"mower", "a double-speed tool", true},
        {"goat", "a tool with double speed for a different tag", false}};

    const auto *grass = &server->findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(grass);

    for (const auto &tool : grassPickingTools) {
      AND_GIVEN("a user has " + tool.description) {
        const auto *item = &server->findItem(tool.id);
        user->giveItem(item);

        WHEN("he starts crafting the recipe") {
          client->sendMessage(CL_CRAFT, makeArgs("grass", 1));

          AND_WHEN("150ms elapses") {
            REPEAT_FOR_MS(150);

            THEN("the product has "s +
                 (tool.canPickGrassAtDoubleSpeed ? "" : "not ") +
                 "been crafted") {
              CHECK(user->hasItems(expectedProduct) ==
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
    useData(R"(
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

    )");

    const auto *grass = &server->findItem("grass");
    auto expectedProduct = ItemSet{};
    expectedProduct.add(grass);

    WHEN("a user starts crafting the recipe") {
      client->sendMessage(CL_CRAFT, makeArgs("grass", 1));

      AND_WHEN("150ms elapses") {
        REPEAT_FOR_MS(150);

        THEN("the product has been crafted") {
          CHECK(user->hasItems(expectedProduct));
        }
      }
    }
  }
}

TEST_CASE("Client sees default recipes", "[crafting]") {
  TestServer s = TestServer::WithData("box_from_nothing");
  TestClient c = TestClient::WithData("box_from_nothing");
  s.waitForUsers(1);

  c.showCraftingWindow();

  CHECK(c.recipeList().size() == 1);
}

TEST_CASE("Crafting is allowed if materials will vacate a slot",
          "[crafting][inventory]") {
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
  c.sendMessage(CL_CRAFT, makeArgs("cookedMeat", 1));
  WAIT_UNTIL(u.action() == User::Action::CRAFT);  // Wait for gathering to start
  WAIT_UNTIL(u.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  // Then his inventory contains cooked meat
  const ServerItem *itemInFirstSlot = u.inventory(0).type();
  REQUIRE(itemInFirstSlot != nullptr);
  CHECK(itemInFirstSlot->id() == "cookedMeat");
}

TEST_CASE("Gear counts towards materials", "[crafting][gear]") {
  GIVEN("A user wearing an item, and a recipe that needs that item") {
    auto data = R"(
      <item id="sock" gearSlot="feet" />
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
    user.gear(5) = {&sock,
                    ServerItem::Instance::ReportingInfo::UserGear(&user, 5), 1};

    WHEN("he tries to craft the recipe") {
      REPEAT_FOR_MS(100);
      c.sendMessage(CL_CRAFT, makeArgs("sockPuppet", 1));

      THEN("he has the new item") {
        auto &sockPuppet = s.findItem("sockPuppet");
        WAIT_UNTIL(user.inventory(0).type() == &sockPuppet);
      }
    }
  }
}

TEST_CASE("Duping exploit", "[crafting][containers]") {
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
      c.sendMessage(CL_CRAFT, makeArgs("brick", 1));

      AND_WHEN("he puts the material into the container") {
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Serial::Inventory(), 0, box.serial(), 0));

        AND_WHEN("enough time passes for the crafting to finish") {
          REPEAT_FOR_MS(150);

          THEN("his inventory is still empty") {
            CHECK_FALSE(user.inventory(0).hasItem());
          }
        }
      }
    }
  }
}

TEST_CASE("Extra items returned from crafting", "[crafting][inventory]") {
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
          c.sendMessage(CL_CRAFT, makeArgs("electricity", 1));

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
          c.sendMessage(CL_CRAFT, makeArgs("electricity", 1));

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
        c.sendMessage(CL_CRAFT, makeArgs("money", 1));

        THEN("he has happiness") {
          auto expectedInInventory = ItemSet{};
          expectedInInventory.add(&happiness);

          WAIT_UNTIL(user.hasItems(expectedInInventory));
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Nonsense recipes can't be added",
                 "[crafting]") {
  WHEN("the user tries to learn a recipe that doesnt exist") {
    user->addRecipe("fakeRecipe");

    THEN("he still knows no recipes") { CHECK(user->knownRecipes().empty()); }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Recipe items", "[crafting]") {
  GIVEN("a recipe item that teaches how to bake a cake") {
    useData(R"(
      <item id="cake" />
      <recipe id="cake"> <unlockedBy/> </recipe>
      <item id="cakeRecipe" castsSpellOnUse="teachRecipe" spellArg="cake" />
    )");

    THEN("the user can't craft anything") {
      CHECK(user->knownRecipes().empty());
    }

    WHEN("the user has the recipe item") {
      auto &cakeRecipe = server->findItem("cakeRecipe");
      user->giveItem(&cakeRecipe);

      AND_WHEN("he uses it") {
        client->sendMessage(CL_CAST_SPELL_FROM_ITEM, makeArgs(0));

        THEN("he can craft something") {
          WAIT_UNTIL(user->knownRecipes().size() == 1);
        }

        THEN("he recieves a message to that effect") {
          CHECK(client->waitForMessage(SV_NEW_RECIPES_LEARNED));
        }
      }

      AND_WHEN("the user already knows the recipe") {
        user->addRecipe("cake");

        AND_WHEN("he tries to use the recipe item") {
          client->sendMessage(CL_CAST_SPELL_FROM_ITEM, makeArgs(0));

          THEN("he still has an item") {
            REPEAT_FOR_MS(100);
            CHECK(user->inventory(0).hasItem());
          }
        }
      }
    }
  }

  GIVEN("a recipe item that teaches how to bake bread") {
    useData(R"(
      <item id="bread" />
      <recipe id="bread"> <unlockedBy/> </recipe>
      <item id="breadRecipe" castsSpellOnUse="teachRecipe" spellArg="bread" />
    )");

    WHEN("the user has the recipe item") {
      auto &breadRecipe = server->findItem("breadRecipe");
      user->giveItem(&breadRecipe);

      AND_WHEN("he uses it") {
        client->sendMessage(CL_CAST_SPELL_FROM_ITEM, makeArgs(0));

        THEN("he can craft something") {
          WAIT_UNTIL(user->knownRecipes().size() == 1);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Crafting quantity", "[crafting]") {
  GIVEN("a recipe to conjur bread from nothing") {
    useData(R"(
      <item id="bread" stackSize="10" />
      <recipe id="bread"/>
    )");
    const auto &invSlot0 = user->inventory(0);
    const auto &invSlot1 = user->inventory(1);

    WHEN("the user crafts 2 with one message") {
      client->sendMessage(CL_CRAFT, makeArgs("bread", 2));

      THEN("he gets 2 bread") { WAIT_UNTIL(invSlot0.quantity() == 2); }
    }

    SECTION("Crafting 'infinite' times") {
      WHEN("the user sends a craft message with quantity 0") {
        client->sendMessage(CL_CRAFT, makeArgs("bread", 0));

        THEN("his inventory gets filled up with bread") {
          WAIT_UNTIL(invSlot1.hasItem());
        }
      }
    }

    SECTION("Ordering from earlier in the client") {
      WHEN("the client's startCrafting() is called") {
        client->startCrafting(&client->getFirstRecipe(), 1);

        THEN("the user gets bread") { WAIT_UNTIL(invSlot0.hasItem()); }
      }
    }

    SECTION("No integer underflow") {
      WHEN("the user sends a craft message with quantity -1") {
        client->sendMessage(CL_CRAFT, makeArgs("bread", -1));

        THEN("his inventory is not full of bread (it may have 1)") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user->inventory(1).hasItem());
        }
      }
    }
  }

  SECTION("items are crafted one at a time") {
    GIVEN("it takes takes 100ms to come up with an idea") {
      useData(R"(
        <item id="idea" stackSize="10" />
        <recipe id="idea" time="100" />
      )");

      WHEN("the user tries to come up with 2 ideas") {
        client->sendMessage(CL_CRAFT, makeArgs("idea", 2));

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("he has one idea") {
            const auto &invSlot = user->inventory(0);
            WAIT_UNTIL(invSlot.quantity() == 1);

            AND_WHEN("another 100ms elapses") {
              REPEAT_FOR_MS(100);

              THEN("he has two ideas") { WAIT_UNTIL(invSlot.quantity() == 2); }
            }
          }
        }
      }
    }
  }

  SECTION("Tools are checked for each crafting action") {
    GIVEN("ideas take 100ms and inspiration") {
      useData(R"(
        <item id="idea" stackSize="10" />
        <recipe id="idea" time="100" >
          <tool class="inspiration" />
        </recipe>
        <objectType id="flower" destroyIfUsedAsTool="1" maxHealth="1000" >
          <tag name="inspiration" />
        </objectType>
      )");

      AND_GIVEN(
          "there is a flower nearby that will get destroyed when used as "
          "inspiration") {
        server->addObject("flower", {10, 15});

        WHEN("he tries to come up with 2 ideas") {
          client->sendMessage(CL_CRAFT, makeArgs("idea", 2));

          AND_WHEN("150ms elapses") {
            REPEAT_FOR_MS(150);

            THEN("he has one idea") {
              const auto &invSlot = user->inventory(0);
              WAIT_UNTIL(invSlot.quantity() == 1);

              AND_WHEN("another 100ms elapses") {
                REPEAT_FOR_MS(100);

                THEN("he still has only one idea") {
                  WAIT_UNTIL(invSlot.quantity() == 1);
                }
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "A byproduct doesn't interfere with a recipe getting unlocked",
                 "[crafting]") {
  GIVEN(
      "crafting a cake unlocks the recipe for a chocolate cake (that returns a "
      "chocolate packet)") {
    useData(R"(
      <item id="cake" />
      <item id="chocolateCake" />
      <item id="chocolatePacket" />
      <recipe id="cake" time="1" />
      <recipe id="chocolatePacket" time="1" />
      <recipe id="chocolateCake" time="1" >
          <material id="cake" />
          <byproduct id="chocolatePacket" />
          <unlockedBy recipe="cake" />
      </recipe>
    )");

    WHEN("the user crafts a cake") {
      client->sendMessage(CL_CRAFT, makeArgs("cake", 1));

      THEN("he has unlocked the split recipe") {
        WAIT_UNTIL(user->knownRecipes().count("chocolateCake") > 0);
      }
    }
  }
}
