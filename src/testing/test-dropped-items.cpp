#include "../server/DroppedItem.h"
#include "TestClient.h"
#include "TestFixtures.h"
#include "testing.h"
#include "TestServer.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Dropping an item creates an object",
                 "[dropped-items]") {
  GIVEN("an item type") {
    useData(R"(
      <item id="apple" />
    )");

    AND_GIVEN("a user has an item") {
      user->giveItem(&server->getFirstItem());

      WHEN("he drops it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("there's an entity") {
          WAIT_UNTIL(server->entities().size() == 1);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Equipped items can be dropped",
                 "[gear][dropped-items]") {
  GIVEN("a gear item") {
    useData(R"(
      <item id="hat" gearSlot="head" />
    )");

    AND_GIVEN("a user has the item equipped") {
      user->giveItem(&server->getFirstItem());
      client->sendMessage(CL_SWAP_ITEMS,
                          makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

      WHEN("he drops it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Gear(), 0));

        THEN("there's an entity") {
          WAIT_UNTIL(server->entities().size() == 1);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Name is correct on client",
                 "[dropped-items][loading]") {
  GIVEN("Apple and Orange item types") {
    useData(R"(
      <item id="apple" name="Apple" />
      <item id="orange" name="Orange" />
    )");

    for (auto pair : client->items()) {
      auto id = pair.first;
      auto name = pair.second.name();

      AND_GIVEN("a user has a " + id) {
        user->giveItem((*server)->findItem(id));

        WHEN("he drops it") {
          client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
          WAIT_UNTIL(client->entities().size() == 2);  // item + player

          THEN("the new entity is named \"" + name + "\"") {
            const auto &di = client->getFirstDroppedItem();
            CHECK(di.name() == name);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Dropped items have correct serials in client",
                 "[dropped-items]") {
  GIVEN("an item type") {
    useData(R"(
      <item id="apple" />
    )");

    AND_GIVEN("a user has an item") {
      user->giveItem(&server->getFirstItem());

      WHEN("he drops it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("the client entity has the correct serial number") {
          WAIT_UNTIL(server->entities().size() == 1);
          const auto &serverEntity = server->getFirstDroppedItem();

          WAIT_UNTIL(client->entities().size() == 2);  // item + player
          const auto &clientEntity = client->getFirstDroppedItem();

          INFO("Client serial = "s + toString(clientEntity.serial()));
          INFO("Server serial = "s + toString(serverEntity.serial()));
          CHECK(clientEntity.serial() == serverEntity.serial());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Dropped items land near the dropping player",
                 "[dropped-items]") {
  GIVEN("an item type") {
    useData(R"(
      <item id="apple" />
    )");
    auto *apple = &server->getFirstItem();

    AND_GIVEN("a user is at a random location") {
      user->teleportTo((*server)->map().randomPoint());

      AND_GIVEN("he has an item") {
        user->giveItem(apple);

        WHEN("he drops it") {
          client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

          THEN("it is within action range of him") {
            WAIT_UNTIL(server->entities().size() == 1);
            const auto &di = server->getFirstDroppedItem();

            auto distanceFromDropper = distance(di, *user);
            CHECK(distanceFromDropper < Server::ACTION_DISTANCE);

            AND_THEN("it is at the same location on the client") {
              WAIT_UNTIL(client->entities().size() == 2);  // item + player
              const auto &clientEntity = client->getFirstDroppedItem();

              CHECK(clientEntity.location() == di.location());
            }
          }
        }
      }
    }

    AND_GIVEN("a user has two items") {
      user->giveItem(apple, 2);

      WHEN("he drops them both") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 1));
        WAIT_UNTIL(server->entities().size() == 2);

        THEN("the two entities have different locations") {
          auto first = true;
          MapPoint loc1, loc2;
          for (auto *e : server->entities()) {
            if (first)
              loc1 = e->location();
            else
              loc2 = e->location();
            first = false;
          }

          CHECK(loc1 != loc2);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Dropped items don't overlap objects",
                 "[dropped-items][construction]") {
  GIVEN("an item type, and a colliding wall near a user") {
    useData(R"(
      <item id="apple" />
      <objectType id="wall">
        <collisionRect x="-10" y="-10" w="20" h="20" />
      </objectType>
    )");
    const auto &wall = server->addObject("wall", {0, 30});

    WHEN("the user drops 100 items") {
      auto *apple = &server->getFirstItem();
      for (auto i = 0; i != 100; ++i) {
        INFO("Dropping item #" << i + 1);
        user->giveItem(apple);
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        REPEAT_FOR_MS(100) {
          if (!user->inventory(0).hasItem()) break;
        }
      }

      THEN("none of them overlap the wall") {
        for (auto *e : server->entities()) {
          if (e == &wall) continue;
          REQUIRE_FALSE(wall.collisionRect().overlaps(e->collisionRect()));
        }
      }
    }
  }

  GIVEN("the user knows how to build a wall") {
    useData(R"(
      <item id="brick" />
      <objectType id="wall" constructionTime="0" >
        <material id="brick" quantity="1" />
        <collisionRect x="-10" y="-10" w="20" h="20" />
      </objectType>
    )");

    WHEN("he drops a brick") {
      auto &brick = server->getFirstItem();
      user->giveItem(&brick);
      client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
      WAIT_UNTIL(server->entities().size() == 1);
      auto &droppedBrick = server->getFirstDroppedItem();

      AND_WHEN("the brick is at {25,25}") {
        droppedBrick.teleportTo({25, 25});

        AND_WHEN("he tries to build a wall at {25,25}") {
          client->sendMessage(CL_CONSTRUCT, makeArgs("wall", 25, 25));

          THEN("there's no wall") {
            REPEAT_FOR_MS(100);
            CHECK(server->entities().size() == 1);
          }
        }
      }

      THEN("he can overlap it") {
        CHECK(droppedBrick.areOverlapsAllowedWith(*user));
      };
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "If nowhere to drop an item, keep it in inventory",
                 "[dropped-items][inventory]") {
  GIVEN("an item type, and a colliding wall all around a user") {
    useData(R"(
      <item id="apple" />
      <objectType id="wall">
        <collisionRect x="-50" y="-50" w="100" h="100" />
      </objectType>
    )");
    const auto &wall = server->addObject("wall", {40, 40});

    WHEN("the user tries drops the item") {
      user->giveItem(&server->getFirstItem());
      client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

      THEN("the user receives a warning") {
        CHECK(client->waitForMessage(WARNING_NOWHERE_TO_DROP_ITEM));

        AND_THEN("he still has it in his inventory") {
          CHECK(user->inventory(0).hasItem());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Picking items back up",
                 "[inventory][dropped-items]") {
  GIVEN("an item type") {
    useData(R"(
      <item id="apple" />
    )");
    const auto *apple = &server->getFirstItem();

    SECTION("Item added and entity removed") {
      AND_GIVEN("a user has dropped one") {
        user->giveItem(apple);
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user->inventory(0).hasItem());

        WHEN("he picks it back up") {
          WAIT_UNTIL(client->entities().size() == 2);
          auto &di = client->getFirstDroppedItem();
          client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

          THEN("he has the item again") {
            WAIT_UNTIL(user->inventory(0).hasItem());

            AND_THEN("the entity is gone") {
              WAIT_UNTIL(server->entities().empty());
              WAIT_UNTIL(client->entities().size() == 1);
            }
          }
        }
      }
    }

    SECTION("Only the specified entity is removed") {
      AND_GIVEN("a user has dropped two of them") {
        user->giveItem(apple, 2);
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 1));

        WHEN("he picks one back up") {
          WAIT_UNTIL(client->entities().size() == 3);
          auto &di = client->getFirstDroppedItem();
          auto serial = di.serial();
          client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(serial));

          THEN("he has the item again") {
            WAIT_UNTIL(user->inventory(0).hasItem());

            AND_THEN("there is still one entity left") {
              WAIT_UNTIL(server->entities().size() == 1);

              AND_THEN("the one he picked up is gone") {
                CHECK((*server)->findEntityBySerial(serial) == nullptr);
              }
            }
          }
        }
      }
    }

    SECTION("Too far away to pick up") {
      AND_GIVEN("a user has dropped one") {
        user->giveItem(apple);
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user->inventory(0).hasItem());

        AND_GIVEN("it's very far away") {
          user->teleportTo({200, 200});

          WHEN("he tries to pick it back up") {
            WAIT_UNTIL(client->entities().size() == 2);
            auto &di = client->getFirstDroppedItem();
            client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

            THEN("he has no item") {
              REPEAT_FOR_MS(100);
              CHECK(!user->inventory(0).hasItem());
            }
          }
        }
      }
    }

    SECTION("Inventory is full") {
      AND_GIVEN("a user has dropped one") {
        user->giveItem(apple);
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user->inventory(0).hasItem());
        WAIT_UNTIL(server->entities().size() == 1);

        AND_GIVEN("he has a full inventory") {
          user->giveItem(apple, User::INVENTORY_SIZE);

          WHEN("he tries to pick up the one he dropped") {
            auto &di = server->getFirstDroppedItem();
            client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

            THEN("the entity still exists on the server") {
              REPEAT_FOR_MS(100);
              CHECK(server->entities().size() == 1);
            }
          }
        }
      }
    }
  }

  SECTION("A different item type") {
    GIVEN("bananas, oranges and plums") {
      useData(R"(
        <item id="banana" />
        <item id="orange" />
        <item id="plum" />
      )");

      AND_GIVEN("a user has dropped an orange") {
        user->giveItem(&server->findItem("orange"));
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user->inventory(0).hasItem());

        WHEN("he picks it back up") {
          WAIT_UNTIL(client->entities().size() == 2);
          auto &di = client->getFirstDroppedItem();
          client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

          THEN("he has an item") {
            WAIT_UNTIL(user->inventory(0).hasItem());

            AND_THEN("it is an orange") {
              CHECK(user->inventory(0).type()->id() == "orange");
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Dropped-item stacks",
                 "[dropped-items]") {
  GIVEN("coins stack to 10") {
    useData(R"(
        <item id="coin" stackSize="10" />
      )");

    for (size_t numCoins : std::vector<size_t>{1, 10}) {
      AND_GIVEN("a player has a stack of " << numCoins) {
        user->giveItem(&server->getFirstItem(), numCoins);

        WHEN("he drops it") {
          client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

          AND_WHEN("he picks it up again") {
            WAIT_UNTIL(server->entities().size() == 1);
            auto serial = server->getFirstDroppedItem().serial();
            client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(serial));

            THEN("he has a stack of " << numCoins) {
              WAIT_UNTIL(user->inventory(0).hasItem());
              CHECK(user->inventory(0).quantity() == numCoins);
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Dropped-item names reflect stack size", "[dropped-items]") {
  GIVEN("coins stack to 10") {
    useData(R"(
        <item id="coin" name="Coin" stackSize="10" />
      )");
    const auto *coin = &server->getFirstItem();

    for (auto numCoins : std::vector<size_t>{5, 10}) {
      WHEN("a player drops a stack of " << numCoins << " coins") {
        user->giveItem(coin, numCoins);
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("its name in the client is \"Coin x" << numCoins << "\"") {
          WAIT_UNTIL(client->entities().size() == 2);
          const auto &clientCoins = client->getFirstDroppedItem();
          CHECK(clientCoins.name() == "Coin x"s + toString(numCoins));
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Bad dropped-item calls",
                 "[dropped-items]") {
  SECTION("Bad serial") {
    GIVEN("No entities") {
      useData(R"(
        <item id="filler" />
      )");

      WHEN("the client calls CL_PICK_UP_DROPPED_ITEM") {
        client->sendMessage(CL_PICK_UP_DROPPED_ITEM, "42"s);

        THEN("the server survives") {}
      }
    }
  }

  SECTION("Wrong object type") {
    GIVEN("a box object") {
      useData(R"(
        <objectType id="box" />
      )");
      const auto &box = server->addObject("box", {10, 15});

      WHEN("a user tries to pick up the box as if it were a dropped item") {
        client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(box.serial()));

        THEN("the server survives") {}
      }
    }
  }
}

TEST_CASE("Dropped items are persistent", "[persistence][dropped-items]") {
  // Given a dropped stack of 5 coins
  auto data = R"(
    <item id="coin" stackSize="10" />
  )";
  {
    auto s = TestServer::WithDataString(data);
    const auto &coin = s.getFirstItem();
    s->addEntity(new DroppedItem(coin, Hitpoints{}, 5, {}, {20, 20}));

    // When the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);

    // Then it's still there
    WAIT_UNTIL(s.entities().size() == 1);
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Dropped items preserve item damage",
                 "[dropped-items][damage-on-use]") {
  GIVEN("a user has an item") {
    useData(R"(
      <item id="box"/>
    )");
    user->giveItem(&server->getFirstItem());

    AND_GIVEN("it is damaged") {
      auto &slot0 = user->inventory(0);
      do {
        slot0.onUseAsTool();
      } while (slot0.isAtFullHealth());
      const auto itemHealth = slot0.health();

      WHEN("he drops it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(server->entities().size() == 1);
        auto &serverDroppedItem = server->getFirstDroppedItem();
        CHECK(serverDroppedItem.itemHealth() == itemHealth);
        WAIT_UNTIL(!slot0.hasItem());

        AND_WHEN("he picks it back up") {
          WAIT_UNTIL(client->entities().size() == 2);
          auto &di = client->getFirstDroppedItem();
          client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

          THEN("it is still damaged") {
            WAIT_UNTIL(slot0.hasItem());
            WAIT_UNTIL(slot0.health() == itemHealth);
          }
        }
      }
    }
  }
}

TEST_CASE("Dropped-item health is persistent",
          "[persistence][dropped-items][damage-on-use]") {
  // For different values of n
  for (const auto testHealth : std::vector<Hitpoints>{5, 6}) {
    INFO("n = "s + toString(testHealth));

    // Given a dropped item with n health
    auto data = R"(
      <item id="thing" ilvl="10"/>
    )";
    {
      auto s = TestServer::WithDataString(data);
      const auto &thing = s.getFirstItem();
      s->addEntity(new DroppedItem(thing, testHealth, 1, {}, {20, 20}));
      const auto &droppedThing = s.getFirstDroppedItem();
      CHECK(droppedThing.itemHealth() == testHealth);

      // When the server restarts
    }
    {
      auto s = TestServer::WithDataStringAndKeepingOldData(data);

      // Then it still has n health
      WAIT_UNTIL(s.entities().size() == 1);
      const auto &thing = s.getFirstDroppedItem();
      CHECK(thing.itemHealth() == testHealth);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Dropped items preserve suffixes",
                 "[dropped-items][suffixes]") {
  GIVEN("swords have either a \"rad\" or \"cool\" suffix") {
    useData(R"(
      <suffixSet id="swordSuffixes" >
        <suffix id="rad"/>
        <suffix id="cool"/>
      </suffixSet>
      <item id="sword" gearSlot="weapon" >
        <randomSuffix fromSet="swordSuffixes" />
      </item>
    )");

    const auto ATTEMPTS = 20;
    for (auto i = 0; i != ATTEMPTS; ++i) {
      AND_GIVEN("the user has a sword with a random suffix") {
        user->giveItem(&server->getFirstItem());
        auto &slot0 = user->inventory(0);
        const auto suffixBefore = slot0.suffix();

        WHEN("he drops it") {
          client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

          AND_WHEN("he picks it back up") {
            WAIT_UNTIL(client->entities().size() == 2);
            auto &di = client->getFirstDroppedItem();
            client->sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));
            WAIT_UNTIL(slot0.hasItem());

            THEN("it still has the same suffix") {
              REQUIRE(slot0.suffix() == suffixBefore);
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Dropped-item suffixes are persistent",
          "[persistence][dropped-items][suffixes]") {
  // For suffixes "rad" and "cool"
  auto data = R"(
      <suffixSet id="swordSuffixes" >
        <suffix id="rad"/>
        <suffix id="cool"/>
      </suffixSet>
      <item id="sword" gearSlot="weapon" >
        <randomSuffix fromSet="swordSuffixes" />
      </item>
  )";

  for (auto suffixID : std::vector<std::string>{"rad"s, "cool"s}) {
    // Given a dropped item with the suffix
    {
      auto s = TestServer::WithDataString(data);
      const auto &sword = s.getFirstItem();
      s->addEntity(
          new DroppedItem(sword, sword.maxHealth(), 1, suffixID, {20, 20}));

      // When the server restarts
    }
    {
      auto s = TestServer::WithDataStringAndKeepingOldData(data);

      // Then it still has the suffix
      WAIT_UNTIL(s.entities().size() == 1);
      const auto &sword = s.getFirstDroppedItem();
      CHECK(sword.suffix() == suffixID);
    }
  }
}
