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
      <item id="hammer" >
        <tag name="pounding" />
      </item>
    )");

    THEN("the client has no tools") {
      REPEAT_FOR_MS(300);
      CHECK(!client->currentTools().hasAnyTools());
    }

    AND_GIVEN("the user has a hacksaw") {
      user->giveItem(&server->findItem("hacksaw"));
      THEN("the client has a sawing tool") {
        WAIT_UNTIL(client->currentTools().hasTool("sawing"));
      }
    }

    SECTION("items' actual tags are used")
    AND_GIVEN("the user has a hammer") {
      user->giveItem(&server->findItem("hammer"));
      THEN("the client has a pounding tool") {
        WAIT_UNTIL(client->currentTools().hasTool("pounding"));
      }
    }

    SECTION("items in all slots are counted") {
      AND_GIVEN("the user has both a hammer and a hacksaw") {
        user->giveItem(&server->findItem("hammer"));
        user->giveItem(&server->findItem("hacksaw"));

        THEN("the client has both a pounding tool and a sawing tool") {
          WAIT_UNTIL(client->currentTools().hasTool("pounding"));
          CHECK(client->currentTools().hasTool("sawing"));
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Client counts multiple tags on an item", "[tool]") {
  GIVEN("a pen that can be used for both writing and stabbing") {
    useData(R"(
      <item id="pen" >
        <tag name="writing" />
        <tag name="stabbing" />
      </item>
    )");

    AND_GIVEN("the user has a pen") {
      user->giveItem(&server->findItem("pen"));

      THEN("the client has both a writing tool and a stabbing tool") {
        WAIT_UNTIL(client->currentTools().hasTool("writing"));
        CHECK(client->currentTools().hasTool("stabbing"));
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Client counts gear tools",
                 "[tool][gear]") {
  GIVEN("an equippable 'mining' pickaxe") {
    useData(R"(
      <item id="pickaxe" gearSlot="weapon" >
        <tag name="mining" />
      </item>
    )");

    AND_GIVEN("the user has a pickaxe equipped") {
      user->giveItem(&server->findItem("pickaxe"));
      client->sendMessage(
          CL_SWAP_ITEMS,
          makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::WEAPON));

      THEN("the client has a mining tool") {
        REPEAT_FOR_MS(300);
        WAIT_UNTIL(client->currentTools().hasTool("mining"));
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Client counts object tools",
                 "[tool]") {
  GIVEN("anvil objects are 'smithing' tools") {
    useData(R"(
      <objectType id="anvil" >
        <tag name="smithing" />
      </objectType>
    )");

    SECTION("Normal case") {
      AND_GIVEN("the user is next to an anvil") {
        server->addObject("anvil", {15, 10});

        THEN("the client has a smithing tool") {
          WAIT_UNTIL(client->currentTools().hasTool("smithing"));
        }
      }
    }

    SECTION("No permission") {
      AND_GIVEN("the user is next to someone else's anvil") {
        server->addObject("anvil", {15, 10}, "Stranger");

        THEN("the client doesn't have a smithing tool") {
          REPEAT_FOR_MS(300);
          CHECK(!client->currentTools().hasTool("smithing"));
        }
      }
    }

    SECTION("Too far away") {
      AND_GIVEN(
          "the user is out of range of an anvil (but the client still knows "
          "about it") {
        server->addObject("anvil", {200, 10});
        WAIT_UNTIL(client->entities().size() == 2);

        THEN("the client doesn't have a smithing tool") {
          REPEAT_FOR_MS(300);
          CHECK(!client->currentTools().hasTool("smithing"));
        }
      }
    }
  }
}
TEST_CASE_METHOD(ServerAndClientWithData,
                 "Client doesn't count construction sites as tools", "[tool]") {
  GIVEN("anvil objects are 'smithing' tools, but require metal to build") {
    useData(R"(
      <item id="metal" />
      <objectType id="anvil" constructionTime="0" >
        <tag name="smithing" />
        <material id="metal" />
      </objectType>
    )");

    AND_GIVEN("the user has begun construction of an anvil") {
      client->sendMessage(CL_CONSTRUCT, makeArgs("anvil", 10, 15));

      THEN("the client doesn't have a smithing tool") {
        REPEAT_FOR_MS(300);
        CHECK(!client->currentTools().hasTool("smithing"));
      }
    }
  }
}

/*
Terrain tool
Terrain with multiple tags
*/
