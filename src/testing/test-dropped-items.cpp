#include "../server/DroppedItem.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Dropping an item creates an object") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    AND_GIVEN("a user has an item") {
      auto &user = s.getFirstUser();
      user.giveItem(&s.getFirstItem());

      WHEN("he drops it") {
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("there's an entity") { WAIT_UNTIL(s.entities().size() == 1); }
      }
    }
  }
}

TEST_CASE("Name is correct on client") {
  GIVEN("Apple and Orange item types") {
    auto data = R"(
      <item id="apple" name="Apple" />
      <item id="orange" name="Orange" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    for (auto pair : c.items()) {
      auto id = pair.first;
      auto name = pair.second.name();

      AND_GIVEN("a user has a " + id) {
        user.giveItem(s->findItem(id));

        WHEN("he drops it") {
          c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
          WAIT_UNTIL(c.entities().size() == 2);  // item + player

          THEN("the new entity is named \"" + name + "\"") {
            const auto &di = c.getFirstDroppedItem();
            CHECK(di.name() == name);
          }
        }
      }
    }
  }
}

TEST_CASE("Dropped items have correct serials in client") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    AND_GIVEN("a user has an item") {
      user.giveItem(&s.getFirstItem());

      WHEN("he drops it") {
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("the client entity has the correct serial number") {
          WAIT_UNTIL(s.entities().size() == 1);
          const auto &serverEntity = s.getFirstDroppedItem();

          WAIT_UNTIL(c.entities().size() == 2);  // item + player
          const auto &clientEntity = c.getFirstDroppedItem();

          INFO("Client serial = "s + toString(clientEntity.serial()));
          INFO("Server serial = "s + toString(serverEntity.serial()));
          CHECK(clientEntity.serial() == serverEntity.serial());
        }
      }
    }
  }
}

TEST_CASE("Dropped items land near the dropping player") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    auto *apple = &s.getFirstItem();

    AND_GIVEN("a user is at a random location") {
      user.teleportTo(s->map().randomPoint());

      AND_GIVEN("he has an item") {
        user.giveItem(apple);

        WHEN("he drops it") {
          c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

          THEN("it is within action range of him") {
            WAIT_UNTIL(s.entities().size() == 1);
            const auto &di = s.getFirstDroppedItem();

            auto distanceFromDropper =
                distance(di.collisionRect(), user.collisionRect());
            CHECK(distanceFromDropper < Server::ACTION_DISTANCE);

            AND_THEN("it is at the same location on the client") {
              WAIT_UNTIL(c.entities().size() == 2);  // item + player
              const auto &clientEntity = c.getFirstDroppedItem();

              CHECK(clientEntity.location() == di.location());
            }
          }
        }
      }
    }

    AND_GIVEN("a user has two items") {
      user.giveItem(apple, 2);

      WHEN("he drops them both") {
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 1));
        WAIT_UNTIL(s.entities().size() == 2);

        THEN("the two entities have different locations") {
          auto first = true;
          MapPoint loc1, loc2;
          for (auto *e : s.entities()) {
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

TEST_CASE("Dropped items don't overlap objects") {
  GIVEN("an item type, and a colliding wall near a user") {
    auto data = R"(
      <item id="apple" />
      <objectType id="wall">
        <collisionRect x="-10" y="-10" w="20" h="20" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    const auto &wall = s.addObject("wall", {0, 30});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("the user drops 100 items") {
      auto *apple = &s.getFirstItem();
      for (auto i = 0; i != 100; ++i) {
        INFO("Dropping item #" << i + 1);
        user.giveItem(apple);
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        REPEAT_FOR_MS(100) {
          if (!user.inventory(0).first.hasItem()) break;
        }
      }

      THEN("none of them overlap the wall") {
        for (auto *e : s.entities()) {
          if (e == &wall) continue;
          REQUIRE_FALSE(wall.collisionRect().overlaps(e->collisionRect()));
        }
      }
    }
  }
}

TEST_CASE("If nowhere to drop an item, keep it in inventory") {
  GIVEN("an item type, and a colliding wall all around a user") {
    auto data = R"(
      <item id="apple" />
      <objectType id="wall">
        <collisionRect x="-50" y="-50" w="100" h="100" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    const auto &wall = s.addObject("wall", {40, 40});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("the user tries drops the item") {
      user.giveItem(&s.getFirstItem());
      c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

      THEN("the user receives a warning") {
        CHECK(c.waitForMessage(WARNING_NOWHERE_TO_DROP_ITEM));

        AND_THEN("he still has it in his inventory") {
          CHECK(user.inventory(0).first.hasItem());
        }
      }
    }
  }
}

TEST_CASE("Picking items back up") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto *apple = &s.getFirstItem();

    SECTION("Item added and entity removed") {
      AND_GIVEN("a user has dropped one") {
        user.giveItem(apple);
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user.inventory(0).first.hasItem());

        WHEN("he picks it back up") {
          WAIT_UNTIL(c.entities().size() == 2);
          auto &di = c.getFirstDroppedItem();
          c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

          THEN("he has the item again") {
            WAIT_UNTIL(user.inventory(0).first.hasItem());

            AND_THEN("the entity is gone") {
              WAIT_UNTIL(s.entities().empty());
              WAIT_UNTIL(c.entities().size() == 1);
            }
          }
        }
      }
    }

    SECTION("Only the specified entity is removed") {
      AND_GIVEN("a user has dropped two of them") {
        user.giveItem(apple, 2);
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 1));

        WHEN("he picks one back up") {
          WAIT_UNTIL(c.entities().size() == 3);
          auto &di = c.getFirstDroppedItem();
          auto serial = di.serial();
          c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(serial));

          THEN("he has the item again") {
            WAIT_UNTIL(user.inventory(0).first.hasItem());

            AND_THEN("there is still one entity left") {
              WAIT_UNTIL(s.entities().size() == 1);

              AND_THEN("the one he picked up is gone") {
                CHECK(s->findEntityBySerial(serial) == nullptr);
              }
            }
          }
        }
      }
    }

    SECTION("Too far away to pick up") {
      AND_GIVEN("a user has dropped one") {
        user.giveItem(apple);
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user.inventory(0).first.hasItem());

        AND_GIVEN("it's very far away") {
          user.teleportTo({200, 200});

          WHEN("he tries to pick it back up") {
            WAIT_UNTIL(c.entities().size() == 2);
            auto &di = c.getFirstDroppedItem();
            c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

            THEN("he has no item") {
              REPEAT_FOR_MS(100);
              CHECK(!user.inventory(0).first.hasItem());
            }
          }
        }
      }
    }

    SECTION("Inventory is full") {
      AND_GIVEN("a user has dropped one") {
        user.giveItem(apple);
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user.inventory(0).first.hasItem());
        WAIT_UNTIL(s.entities().size() == 1);

        AND_GIVEN("he has a full inventory") {
          user.giveItem(apple, User::INVENTORY_SIZE);

          WHEN("he tries to pick up the one he dropped") {
            auto &di = s.getFirstDroppedItem();
            c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

            THEN("the entity still exists on the server") {
              REPEAT_FOR_MS(100);
              CHECK(s.entities().size() == 1);
            }
          }
        }
      }
    }
  }

  SECTION("A different item type") {
    GIVEN("bananas, oranges and plums") {
      auto data = R"(
        <item id="banana" />
        <item id="orange" />
        <item id="plum" />
      )";
      auto s = TestServer::WithDataString(data);
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      AND_GIVEN("a user has dropped an orange") {
        user.giveItem(&s.findItem("orange"));
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
        WAIT_UNTIL(!user.inventory(0).first.hasItem());

        WHEN("he picks it back up") {
          WAIT_UNTIL(c.entities().size() == 2);
          auto &di = c.getFirstDroppedItem();
          c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(di.serial()));

          THEN("he has an item") {
            WAIT_UNTIL(user.inventory(0).first.hasItem());

            AND_THEN("it is an orange") {
              CHECK(user.inventory(0).first.type()->id() == "orange");
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Dropped-item stacks") {
  GIVEN("coins stack to 10") {
    auto data = R"(
        <item id="coin" stackSize="10" />
      )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    for (auto numCoins : std::vector<size_t>{1, 10}) {
      AND_GIVEN("a player has a stack of " << numCoins) {
        user.giveItem(&s.getFirstItem(), numCoins);

        WHEN("he drops it") {
          c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

          AND_WHEN("he picks it up again") {
            WAIT_UNTIL(s.entities().size() == 1);
            auto serial = s.getFirstDroppedItem().serial();
            c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(serial));

            THEN("he has a stack of " << numCoins) {
              WAIT_UNTIL(user.inventory(0).first.hasItem());
              CHECK(user.inventory(0).second == numCoins);
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Dropped-item names reflect stack size") {
  GIVEN("coins stack to 10") {
    auto data = R"(
        <item id="coin" name="Coin" stackSize="10" />
      )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto *coin = &s.getFirstItem();

    for (auto numCoins : std::vector<size_t>{5, 10}) {
      WHEN("a player drops a stack of " << numCoins << " coins") {
        user.giveItem(coin, numCoins);
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("its name in the client is \"Coin x" << numCoins << "\"") {
          WAIT_UNTIL(c.entities().size() == 2);
          const auto &clientCoins = c.getFirstDroppedItem();
          CHECK(clientCoins.name() == "Coin x"s + toString(numCoins));
        }
      }
    }
  }
}

TEST_CASE("Bad dropped-item calls") {
  SECTION("Bad serial") {
    GIVEN("No entities") {
      auto s = TestServer{};
      auto c = TestClient{};
      s.waitForUsers(1);

      WHEN("the client calls CL_PICK_UP_DROPPED_ITEM") {
        c.sendMessage(CL_PICK_UP_DROPPED_ITEM, "42"s);

        THEN("the server survives") {}
      }
    }
  }

  SECTION("Wrong object type") {
    GIVEN("a box object") {
      auto data = R"(
        <objectType id="box" />
      )";
      auto s = TestServer::WithDataString(data);
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      const auto &box = s.addObject("box", {10, 15});

      WHEN("a user tries to pick up the box as if it were a dropped item") {
        c.sendMessage(CL_PICK_UP_DROPPED_ITEM, makeArgs(box.serial()));

        THEN("the server survives") {}
      }
    }
  }
}
