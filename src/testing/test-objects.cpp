#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Damaged objects can't be deconstructed", "[damage-on-use]") {
  GIVEN("a 'brick' object with 1 out of 2 health") {
    useData(R"(
      <item id="brick" />
      <objectType id="brick" deconstructs="brick" maxHealth="2" />
    )");

    auto &brick = server->addObject("brick", {10, 15});
    brick.reduceHealth(1);
    REQUIRE(brick.health() == 1);

    WHEN("the user tries to deconstruct the brick") {
      client->sendMessage(CL_PICK_UP_OBJECT_AS_ITEM, makeArgs(brick.serial()));

      THEN("the object still exists") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(server->entities().empty());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Specifying object health",
                 "[loading][stats]") {
  GIVEN("saplings have 2 max health") {
    useData(R"(
      <objectType id="sapling" maxHealth="2" />
    )");

    WHEN("a sapling is created") {
      const auto &serverObject = server->addObject("sapling", {10, 10});

      THEN("it has 2 health") { CHECK(serverObject.health() == 2); }

      WHEN("the client becomes aware of it") {
        WAIT_UNTIL(client->objects().size() == 1);
        const auto &clientObject = client->getFirstObject();

        THEN("the client knows it has 2 health") {
          CHECK(clientObject.health() == 2);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Objects have 1 health by default",
                 "[loading][stats]") {
  GIVEN("an object with no explicit health") {
    useData(R"(
      <objectType id="A" />
    )");

    const auto &serverObject = server->addObject("A", {10, 15});

    THEN("it has 1 health") { CHECK(serverObject.health() == 1); }

    WHEN("the client becomes aware of it") {
      WAIT_UNTIL(client->objects().size() == 1);
      const auto &clientObject = client->getFirstObject();

      THEN("the client knows it has 1 health") {
        CHECK(clientObject.health() == 1);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Objects are level 1 by default",
                 "[loading]") {
  GIVEN("an object") {
    useData(R"(
      <objectType id="A" />
    )");
    const auto &serverObject = server->addObject("A", {10, 15});

    THEN("it is level 1") {
      CHECK(serverObject.level() == 1);

      AND_THEN("it's level 1 on the client") {
        CHECK(client->waitForFirstObject().level() == 1);
      }
    }
  }
}

TEST_CASE("Objects that disappear after a time") {
  GIVEN("an object that disappears after 1s") {
    auto data = R"(
      <objectType id="A" disappearAfter="1000" />
    )";
    TestServer s = TestServer::WithDataString(data);
    TestClient c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});

    WHEN("0.9s elapses") {
      REPEAT_FOR_MS(900);

      THEN("There is an object") { CHECK(s.entities().size() == 1); }

      AND_WHEN("Another 0.2s elapses") {
        REPEAT_FOR_MS(200);

        THEN("There are no objects") { CHECK(s.entities().size() == 0); }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Object-health categories",
                 "[stats]") {
  GIVEN("a house type whose health category specifies 100 health") {
    useData(R"(
      <objectHealthCategory id="building" maxHealth="100" />
      <objectType id="house" healthCategory="building" />
    )");

    WHEN("a house is spawned") {
      const auto &house = server->addObject("house", {10, 10});

      THEN("it has 100 health") {
        CHECK(house.health() == 100);

        AND_THEN("it has 100 health on the client") {
          CHECK(client->waitForFirstObject().health() == 100);
        }
      }
    }
  }

  GIVEN("a flower type whose health category specifies 2 health") {
    useData(R"(
      <objectHealthCategory id="plant" maxHealth="2" />
      <objectType id="flower" healthCategory="plant" />
    )");

    WHEN("a flower is spawned") {
      const auto &flower = server->addObject("flower", {10, 10});

      THEN("it has 2 health") {
        CHECK(flower.health() == 2);

        AND_THEN("it has 2 health on the client") {
          CHECK(client->waitForFirstObject().health() == 2);
        }
      }
    }
  }

  SECTION("Two categories are defined") {
    GIVEN("straw houses and brick houses have different health categories") {
      useData(R"(
        <objectHealthCategory id="straw" maxHealth="1" />
        <objectHealthCategory id="brick" maxHealth="10" />
        <objectType id="strawHouse" healthCategory="straw" />
        <objectType id="brickHouse" healthCategory="brick" />
      )");

      WHEN("both are spawned") {
        const auto &strawHouse = server->addObject("strawHouse", {10, 10});
        const auto &brickHouse = server->addObject("brickHouse", {10, 20});

        THEN("they have different health") {
          CHECK(strawHouse.health() != brickHouse.health());

          AND_THEN("they have different health on the client") {
            WAIT_UNTIL(client->objects().size() == 2);
            const auto &cStrawHouse = *client->objects()[strawHouse.serial()];
            const auto &cBrickHouse = *client->objects()[brickHouse.serial()];
            CHECK(cStrawHouse.health() != cBrickHouse.health());
          }
        }
      }
    }
  }

  SECTION("Default health for nonexistent categories") {
    GIVEN("a house type with a fake health category") {
      useData(R"(
        <objectType id="house" healthCategory="building" />
      )");

      WHEN("a house is spawned") {
        const auto &house = server->addObject("house", {10, 10});

        THEN("it has 1 health") {
          CHECK(house.health() == 1);

          AND_THEN("it has 1 health on the client") {
            CHECK(client->waitForFirstObject().health() == 1);
          }
        }
      }
    }
  }

  SECTION("Levels") {
    GIVEN("a house type with a category specifying level 2") {
      useData(R"(
        <objectHealthCategory id="building" level="2" />
        <objectType id="house" healthCategory="building" />
      )");

      WHEN("a house is spawned") {
        const auto &house = server->addObject("house", {10, 10});

        THEN("it is level 2") {
          CHECK(house.level() == 2);

          AND_THEN("it's level 2 on the client") {
            CHECK(client->waitForFirstObject().level() == 2);
          }
        }
      }
    }

    GIVEN("a house type with a category specifying level 3") {
      useData(R"(
        <objectHealthCategory id="building" level="3" />
        <objectType id="house" healthCategory="building" />
      )");

      WHEN("a house is spawned") {
        const auto &house = server->addObject("house", {10, 10});

        THEN("it is level 3") {
          CHECK(house.level() == 3);

          AND_THEN("it's level 2 on the client") {
            CHECK(client->waitForFirstObject().level() == 3);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "Object naming", "[permissions]") {
  GIVEN("Alice owns a dog") {
    const auto data = R"(
      <npcType id="dog" name="Dog" />
    )";
    useData(data);
    auto &dog = server->addNPC("dog", {15, 15});
    dog.permissions.setPlayerOwner("Alice");

    THEN("it's named the default, \"Dog\"") {
      const auto &bobDog = cBob->waitForFirstNPC();
      REPEAT_FOR_MS(100);
      CHECK(bobDog.name() == "Dog");
    }

    AND_WHEN("Charlie logs in nearby") {
      auto cCharlie = TestClient::WithUsernameAndDataString("Charlie", data);
      server->waitForUsers(3);

      THEN("it has the default name") {
        const auto &charlieDog = cCharlie.waitForFirstNPC();
        REPEAT_FOR_MS(100);
        CHECK(charlieDog.name() == "Dog");
      }
    }

    WHEN("she names it \"Rex\"") {
      cAlice->sendMessage(CL_SET_OBJECT_NAME, makeArgs(dog.serial(), "Rex"));

      THEN("Bob knows its name") {
        const auto &bobDog = cBob->waitForFirstNPC();
        WAIT_UNTIL(bobDog.name() == "Rex");

        AND_WHEN("Charlie logs in nearby") {
          auto cCharlie =
              TestClient::WithUsernameAndDataString("Charlie", data);
          server->waitForUsers(3);

          THEN("he knows its name") {
            const auto &charlieDog = cCharlie.waitForFirstNPC();
            WAIT_UNTIL(charlieDog.name() == "Rex");
          }
        }
      }
    }

    SECTION("propagated object info includes correct name") {
      WHEN("she names it \"Spot\"") {
        cAlice->sendMessage(CL_SET_OBJECT_NAME, makeArgs(dog.serial(), "Spot"));

        AND_WHEN("Charlie logs in nearby") {
          auto cCharlie =
              TestClient::WithUsernameAndDataString("Charlie", data);
          server->waitForUsers(3);

          THEN("he knows it's named \"Spot\"") {
            const auto &charlieDog = cCharlie.waitForFirstNPC();
            WAIT_UNTIL(charlieDog.name() == "Spot");
          }
        }
      }
    }

    SECTION("Target must be nearby") {
      AND_GIVEN("she is out of range of the dog") {
        dog.ai.giveOrder(AI::ORDER_TO_STAY);
        uAlice->teleportTo({150, 0});
        REPEAT_FOR_MS(100);

        WHEN("she tries to name it") {
          const auto &aliceDog = cAlice->waitForFirstNPC();
          cAlice->sendMessage(CL_SET_OBJECT_NAME,
                              makeArgs(dog.serial(), "Rex"));

          THEN("it is still named \"Dog\"") {
            REPEAT_FOR_MS(300);
            CHECK(aliceDog.name() == "Dog");
          }

          THEN("she gets a warning") {
            CHECK(cAlice->waitForMessage(WARNING_TOO_FAR));
          }
        }
      }
    }

    SECTION("User must have permission") {
      WHEN("Bob tries to name it") {
        cBob->sendMessage(CL_SET_OBJECT_NAME, makeArgs(dog.serial(), "Rex"));

        THEN("it is still named \"Dog\"") {
          REPEAT_FOR_MS(300);
          const auto &aliceDog = cAlice->waitForFirstNPC();
          CHECK(aliceDog.name() == "Dog");
        }
      }
    }
  }

  SECTION("Plain objects can't be named") {
    GIVEN("Alice has a simple rock object") {
      useData(R"(
        <objectType id="rock" name="Rock" />
      )");
      auto &rock = server->addObject("rock", {15, 15}, "Alice");

      WHEN("she tries to rename it") {
        cAlice->sendMessage(CL_SET_OBJECT_NAME,
                            makeArgs(rock.serial(), "Rocky"));

        THEN("it's still called \"Rock\"") {
          const auto &aliceRock = cAlice->waitForFirstObject();
          REPEAT_FOR_MS(100);
          CHECK(aliceRock.name() == "Rock");
        }
      }
    }
  }

  SECTION("Merchant objects can be renamed") {
    GIVEN("Alice has a merchant object") {
      useData(R"(
        <objectType id="shop" name="Shop" merchantSlots="1" />
      )");
      auto &shop = server->addObject("shop", {15, 15}, "Alice");

      WHEN("she tries to rename it") {
        cAlice->sendMessage(CL_SET_OBJECT_NAME,
                            makeArgs(shop.serial(), "Caps for Sale"));

        THEN("it has the new name") {
          const auto &aliceShop = cAlice->waitForFirstObject();
          WAIT_UNTIL(aliceShop.name() == "Caps for Sale");
        }
      }
    }
  }

  SECTION("Resetting") {
    GIVEN("Alice has a merchant object named 'Custom Shop'") {
      useData(R"(
        <objectType id="shop" name="Shop" merchantSlots="1" />
      )");
      auto &shop = server->addObject("shop", {15, 15}, "Alice");
      shop.setCustomNameWithChecksAndAnnouncements("Custom Shop", *uAlice);
      auto &aliceShop = cAlice->waitForFirstObject();
      WAIT_UNTIL(aliceShop.name() == "Custom Shop");

      WHEN("she resets its custom name") {
        cAlice->sendMessage(CL_CLEAR_OBJECT_NAME, makeArgs(shop.serial()));

        THEN("it has its default name once again") {
          WAIT_UNTIL(aliceShop.name() == "Shop");
        }
      }
    }
  }

  SECTION("Max length") {
    GIVEN("Alice has a merchant object") {
      useData(R"(
        <objectType id="shop" name="Shop" merchantSlots="1" />
      )");
      auto &shop = server->addObject("shop", {15, 15}, "Alice");

      WHEN("she tries to rename it to a very long name") {
        const auto attemptedName =
            "This is a very long name, which should exceed the character limit for custom object names."s;
        cAlice->sendMessage(CL_SET_OBJECT_NAME,
                            makeArgs(shop.serial(), attemptedName));

        THEN("its actual name is shorter") {
          const auto &aliceShop = cAlice->waitForFirstObject();
          WAIT_UNTIL(aliceShop.name() != "Shop");
          CHECK(aliceShop.name().size() < attemptedName.size());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Renaming nonexistent objects") {
  WHEN("the user tries to rename a nonexistent object") {
    client.sendMessage(CL_SET_OBJECT_NAME, makeArgs(1337, "Irrelevant"));
  }
  WHEN("the user tries to clear a nonexistent object's name") {
    client.sendMessage(CL_CLEAR_OBJECT_NAME, makeArgs(1337));
  }

  // THEN the client receives a warning message
  CHECK(client.waitForMessage(WARNING_DOESNT_EXIST));
}

TEST_CASE_METHOD(TwoClientsWithData,
                 "Distant citizens know when a city-owned object is renamed",
                 "[city]") {
  GIVEN("Bob is very far away from Alice") {
    useData(R"(
      <terrain index="G" id="grass" />
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="40" y="1" />
      <row    y="0" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />

      <objectType id="shop" name="Shop" merchantSlots="1" />
    )");
    uBob->teleportTo({1200, 0});

    GIVEN("Alice and Bob are in a city") {
      server->cities().createCity("Athens", {}, {});
      server->cities().addPlayerToCity(*uAlice, "Athens");
      server->cities().addPlayerToCity(*uBob, "Athens");

      AND_GIVEN("the city owns a merchant object near Alice") {
        auto &shop = server->addObject("shop", {5, 5});
        shop.permissions.setCityOwner("Athens");

        WHEN("Alice renames the object") {
          cAlice->sendMessage(CL_SET_OBJECT_NAME,
                              makeArgs(shop.serial(), "Caps for Sale"));

          THEN("Bob knows") {
            const auto &bobShop = cBob->waitForFirstObject();
            WAIT_UNTIL(bobShop.name() == "Caps for Sale");
          }
        }
      }
    }
  }
}

TEST_CASE("Custom object names are persistent") {
  // GIVEN an object and an NPC with custom names
  auto data = R"(
    <objectType id="shop" name="Shop" merchantSlots="1" />
    <npcType id="dog" name="Dog" />
  )";
  {
    auto server = TestServer::WithDataString(data);

    auto &shop = server.addObject("shop", {10, 15});
    shop.setCustomName("Fancy Hats");

    auto &dog = server.addNPC("dog", {15, 10});
    dog.setCustomName("Rex");

    // When the server restarts
  }
  {
    auto server = TestServer::WithDataStringAndKeepingOldData(data);

    // Then they still has the custom names
    auto &shop = server.getFirstObject();
    CHECK((shop.customName() == "Fancy Hats"));

    auto &dog = server.getFirstNPC();
    CHECK((dog.customName() == "Rex"));
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "A type can explicitly allow custom names") {
  GIVEN("a statue that can be renamed") {
    useData(R"(
      <objectType id="statue" name="Statue" allowsCustomName="1" />
    )");
    auto &statue = server->addObject("statue", {10, 15});

    WHEN("a user tries to rename it") {
      client->sendMessage(CL_SET_OBJECT_NAME,
                          makeArgs(statue.serial(), "Some Guy"));

      THEN("it has the new name") {
        const auto &clientStatue = client->waitForFirstObject();
        WAIT_UNTIL(clientStatue.name() == "Some Guy");
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Hidden objects") {
  GIVEN("a user owns a hidden ward object") {
    const auto data = R"(
      <objectType id="ward" hidden="1" />
    )";
    useData(data);
    server->addObject("ward", {10, 15}, user->name());

    WHEN("another user logs in") {
      auto newUser = TestClient::WithDataString(data);
      server->waitForUsers(2);

      THEN("he doesn't know about the ward") {
        REPEAT_FOR_MS(500);
        CHECK(newUser.objects().size() == 0);
      }
    }
  }

  SECTION("Approaching hidden objects") {
    GIVEN("the user is very far away from an unowned ward") {
      useData(R"(
      <terrain index="G" id="grass" />
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="40" y="1" />
      <row    y="0" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />

      <objectType id="ward" hidden="1" />
    )");
      server->addObject("ward", {1200, 0});

      WHEN("he walks towards it") {
        auto startTime = SDL_GetTicks();
        while (true) {
          const auto reachedTarget = user->location().x >= 1200;
          if (reachedTarget) {
            break;
          }

          const auto reachedTimeout = SDL_GetTicks() - startTime > 20000;
          if (reachedTimeout) {
            break;
          }

          client->sendMessage(CL_MOVE_TO, makeArgs(1200, 10));

          // THEN he doesn't find out about it
          REQUIRE(client->objects().size() == 0);
          SDL_Delay(5);
        }
      }
    }
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "Newly built hidden objects are hidden") {
  GIVEN("a hidden object type") {
    useData(R"(
      <objectType id="ward" hidden="1" />
    )");

    WHEN("Alice gets a new one") {
      server->addObject("ward", {15, 15}, "Alice");

      THEN("Bob doesn't know about it") {
        REPEAT_FOR_MS(1000);
        CHECK(cBob->objects().empty());
      }
    }
  }
}
