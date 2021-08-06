#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping items", "[scrapping]") {
  GIVEN("wood that can be scrapped into a woodchip") {
    useData(R"(
      <item id="wood" class="wood" />
      <item id="woodchip" />
      <itemClass id="wood">
        <canBeScrapped result="woodchip" />
      </itemClass>
    )");
    const auto &wood = server->findItem("wood");
    const auto &woodchip = server->findItem("woodchip");

    AND_GIVEN("the user has wood") {
      user->giveItem(&wood);

      WHEN("he scraps it") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he has a wood chip") {
          WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
        }
      }
    }

    SECTION("Different slot") {
      AND_GIVEN("the user has wood in slot 5") {
        user->giveItem(&wood);
        client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                    Serial::Inventory(), 5));

        WHEN("he scraps it") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 5));

          THEN("he has a wood chip") {
            WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
          }
        }
      }
    }

    SECTION("Correct slot is scrapped") {
      AND_GIVEN("the user has wood in inventory slots 0-2") {
        user->giveItem(&wood, 3);

        WHEN("he scraps the wood in slot 1") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 1));

          THEN("he has a wood chip in slot 1") {
            WAIT_UNTIL(user->inventory()[1].first.type() == &woodchip);
          }
        }
      }
    }
  }

  SECTION("Different items") {
    GIVEN("a rock that can be scrapped into sand") {
      useData(R"(
        <item id="rock" class="rock" />
        <item id="sand" />
        <itemClass id="rock">
          <canBeScrapped result="sand" />
        </itemClass>
      )");

      AND_GIVEN("the user has a rock") {
        const auto &rock = server->findItem("rock");
        user->giveItem(&rock);

        WHEN("he scraps it") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("he has sand") {
            const auto &sand = server->findItem("sand");
            WAIT_UNTIL(user->inventory()[0].first.type() == &sand);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Normal-distribution yield from scrapping", "[scrapping]") {
  GIVEN("wood can be scrapped into 1-3 woodchips") {
    useData(R"(
        <item id="wood" class="wood" stackSize="100" />
        <item id="woodchip" stackSize="1000" />
        <itemClass id="wood">
          <canBeScrapped result="woodchip" mean="2" sd="0.5" />
        </itemClass>
      )");

    AND_GIVEN("the user has 100 wood") {
      const auto &wood = server->findItem("wood");
      user->giveItem(&wood, 100);

      SECTION("Scrapping a stack only removes one item") {
        WHEN("he scraps one wood") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("he has both wood and woodchips") {
            const auto &woodchip = server->findItem("woodchip");
            WAIT_UNTIL(user->inventory()[1].first.type() == &woodchip);
            CHECK(user->inventory()[0].first.type() == &wood);
          }
        }

        WHEN("he scraps all 100 wood") {
          for (auto i = 0; i != 100; ++i)
            client->sendMessage(CL_SCRAP_ITEM,
                                makeArgs(Serial::Inventory(), 0));

          THEN("he has no wood left") {
            WAIT_UNTIL(!user->inventory()[0].first.hasItem());
          }
        }
      }
    }
  }

  GIVEN("a circle can be scrapped into two semicircles") {
    useData(R"(
        <item id="circle" class="circle" />
        <item id="semicircle" stackSize="2" />
        <itemClass id="circle">
          <canBeScrapped result="semicircle" mean="2" sd="0" />
        </itemClass>
      )");
    const auto *circle = &server->findItem("circle");
    const auto *semicircle = &server->findItem("semicircle");

    WHEN("the user scraps a circle") {
      user->giveItem(circle);
      client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

      THEN("he has two semicircles") {
        WAIT_UNTIL(user->inventory()[0].second == 2);
      }
    }
  }

  SECTION("Randomness") {
    GIVEN("three different coins, scrappable into random numbers of dust") {
      useData(R"(
        <item id="goldCoin" class="gold" />
        <item id="silverCoin" class="silver" />
        <item id="copperCoin" class="copper" />

        <item id="goldDust" stackSize="1000000" />
        <item id="silverDust" stackSize="1000000" />
        <item id="copperDust" stackSize="1000000" />

        <itemClass id="gold">
          <canBeScrapped result="goldDust" mean="100000" sd="20000" />
        </itemClass>
        <itemClass id="silver">
          <canBeScrapped result="silverDust" mean="100000" sd="20000" />
        </itemClass>
        <itemClass id="copper">
          <canBeScrapped result="copperDust" mean="100000" sd="20000" />
        </itemClass>
      )");

      AND_GIVEN("the player has one of each") {
        user->giveItem(&server->findItem("goldCoin"));
        user->giveItem(&server->findItem("silverCoin"));
        user->giveItem(&server->findItem("copperCoin"));

        WHEN("each is scrapped") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 1));
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 2));
          WAIT_UNTIL(user->inventory()[2].first.type() ==
                     &server->findItem("copperDust"));

          THEN("the quantities of scraps are different") {
            const auto qtyGoldDust = user->inventory()[0].second;
            const auto qtySilverDust = user->inventory()[1].second;
            const auto qtyCopperDust = user->inventory()[2].second;

            const auto allEqual =
                qtyGoldDust == qtySilverDust && qtySilverDust == qtyCopperDust;
            CHECK_FALSE(allEqual);
          }
        }
      }
    }
  }

  SECTION("Never negative") {
    GIVEN("an item that always yields a negative number of scraps") {
      useData(R"(
        <item id="item" class="item" />
        <item id="scrap" />
        <itemClass id="item">
          <canBeScrapped result="scrap" mean="-10" sd="0" />
        </itemClass>
      )");

      WHEN("the user scraps one") {
        user->giveItem(&server->findItem("item"));
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he has nothing in inventory") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user->inventory()[0].first.hasItem());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping Equipped gear",
                 "[scrapping][gear]") {
  GIVEN("a hat that can be scrapped into felt") {
    useData(R"(
      <item id="hat" class="hat" gearSlot="0" />
      <item id="felt" />
      <itemClass id="hat">
        <canBeScrapped result="felt" />
      </itemClass>
    )");

    AND_GIVEN("the user is wearing one") {
      const auto &hat = server->findItem("hat");
      user->giveItem(&hat);
      client->sendMessage(CL_SWAP_ITEMS,
                          makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

      WHEN("he scraps it") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Gear(), 0));

        THEN("he has felt") {
          const auto &felt = server->findItem("felt");
          WAIT_UNTIL(user->inventory()[0].first.type() == &felt);
        }

        SECTION("user gets inventory message for scrapped item") {
          THEN("he knows he isn't wearing a hat") {
            REPEAT_FOR_MS(100);
            CHECK(client->gear()[0].first.type() == nullptr);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping item in container",
                 "[scrapping][container]") {
  GIVEN("an egg that can be scrapped into a shell, and a carton container") {
    useData(R"(
      <item id="egg" class="egg" />
      <item id="shell" />
      <itemClass id="egg">
        <canBeScrapped result="shell" />
      </itemClass>
      <objectType id="carton">
        <container slots="1"/>
      </objectType>
    )");

    AND_GIVEN("a carton containing an egg") {
      auto &carton = server->addObject("carton");
      const auto &egg = server->findItem("egg");
      carton.container().addItems(&egg);

      AND_GIVEN("the carton belongs to the player") {
        carton.permissions.setPlayerOwner(user->name());

        WHEN("he scraps the egg") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(carton.serial(), 0));

          THEN("it is empty") {
            REPEAT_FOR_MS(100);
            CHECK(carton.container().isEmpty());
          }
        }

        SECTION("Bad slot number") {
          WHEN("he tries scrapping from an invalid container slot") {
            client->sendMessage(CL_SCRAP_ITEM, makeArgs(carton.serial(), 1));

            THEN("he gets a warning") {
              CHECK(client->waitForMessage(ERROR_INVALID_SLOT));
            }
          }
        }

        SECTION("Too far away") {
          AND_GIVEN("he is far away from the carton") {
            user->teleportTo({300, 300});

            WHEN("he tries scrapping the egg") {
              client->sendMessage(CL_SCRAP_ITEM, makeArgs(carton.serial(), 0));

              THEN("the carton still has the egg") {
                REPEAT_FOR_MS(100);
                CHECK_FALSE(carton.container().isEmpty());
              }

              THEN("he gets a warning") {
                CHECK(client->waitForMessage(WARNING_TOO_FAR));
              }
            }
          }
        }
      }

      SECTION("No permission to use container") {
        AND_GIVEN("nobody has access to it") {
          carton.permissions.setNoAccess();

          WHEN("the user tries to scrap the egg") {
            client->sendMessage(CL_SCRAP_ITEM, makeArgs(carton.serial(), 0));

            THEN("the carton still has the egg") {
              REPEAT_FOR_MS(100);
              CHECK_FALSE(carton.container().isEmpty());
            }

            THEN("he gets a warning") {
              CHECK(client->waitForMessage(WARNING_NO_PERMISSION));
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapped items must be scrappable",
                 "[scrapping]") {
  GIVEN("the user has an item with no item class") {
    useData("<item id=\"diamond\"/>");
    const auto *diamond = &server->getFirstItem();
    user->giveItem(diamond);

    WHEN("he tries to scrap it") {
      client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

      THEN("he still has it") {
        REPEAT_FOR_MS(100);
        CHECK(user->inventory()[0].first.type() == diamond);
      }

      THEN("he gets a warning") {
        CHECK(client->waitForMessage(WARNING_NOT_SCRAPPABLE));
      }
    }
  }

  GIVEN("the user has an item with a non-scrappable item class") {
    useData(R"(
      <item id="diamond" class="unbreakable" />
      <itemClass id="unbreakable" />
    )");

    const auto *diamond = &server->getFirstItem();
    user->giveItem(diamond);

    WHEN("he tries to scrap it") {
      client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

      THEN("he still has it") {
        REPEAT_FOR_MS(100);
        CHECK(user->inventory()[0].first.type() == diamond);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Scrapping is prevented when no inventory space",
                 "[scrapping]") {
  GIVEN("coins stack to 10 and can be scrapped into gold dust") {
    useData(R"(
      <item id="coin" class="gold" stackSize="10"/>
      <item id="goldDust" />
      <itemClass id="gold">
        <canBeScrapped result="goldDust" />
      </itemClass>
    )");

    AND_GIVEN("the user's inventory is full of full stacks of coins") {
      const auto *coin = &server->findItem("coin");
      user->giveItem(coin, 10 * User::INVENTORY_SIZE);

      WHEN("he tries to scrap one") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he hasn't lost any coins") {
          REPEAT_FOR_MS(100);
          CHECK(user->inventory()[0].second == 10);  // Still a full stack
        }

        THEN("he receives a warning") {
          CHECK(client->waitForMessage(WARNING_INVENTORY_FULL));
        }
      }
    }
  }

  SECTION("Quantity > 1") {
    GIVEN("a wonka bar that can be scrapped into a million pieces") {
      useData(R"(
        <item id="wonkaBar" class="transmissable"/>
        <item id="piece" />
        <itemClass id="transmissable">
          <canBeScrapped result="piece" mean="1000000" />
        </itemClass>
      )");
      const auto *wonkaBar = &server->findItem("wonkaBar");

      WHEN("the user tries to scrap one") {
        user->giveItem(wonkaBar);
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he still has the wonka bar") {
          REPEAT_FOR_MS(100);
          CHECK(user->inventory()[0].first.type() == wonkaBar);
        }
      }
    }
  }

  SECTION("When scrapping gear, it doesn't make room for the result") {
    GIVEN("a hat can be scrapped into straw") {
      useData(R"(
        <item id="hat" class="hat" gearSlot="0"/>
        <item id="straw" />
        <itemClass id="hat">
          <canBeScrapped result="straw" />
        </itemClass>
      )");

      AND_GIVEN("the user is wearing one") {
        const auto *hat = &server->findItem("hat");
        user->giveItem(hat);
        client->sendMessage(
            CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));
        WAIT_UNTIL(!user->inventory()[0].first.hasItem());

        AND_GIVEN("he has an inventory full of them") {
          user->giveItem(hat, User::INVENTORY_SIZE);

          WHEN("he tries scrapping the one he's wearing") {
            client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Gear(), 0));

            THEN("he gets a full-inventory warning") {
              CHECK(client->waitForMessage(WARNING_INVENTORY_FULL));
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Rounding of scrapping quantity",
                 "[scrapping]") {
  GIVEN("Scrapping a matrioshka doll gives ~0.999999 matrioshka dolls") {
    useData(R"(
      <item id="matrioshkaDoll" class="matrioshkaDoll"/>
      <itemClass id="matrioshkaDoll">
        <canBeScrapped result="matrioshkaDoll" mean="0.999999" />
      </itemClass>
    )");

    WHEN("the user scraps one") {
      user->giveItem(&server->getFirstItem());
      client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

      THEN("he has a matrioshka doll") {
        REPEAT_FOR_MS(100);
        CHECK(user->inventory()[0].first.hasItem());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Bad scrap result", "[scrapping]") {
  GIVEN("the user has an item that scraps to an invalid item") {
    useData(R"(
      <item id="box" class="box"/>
      <itemClass id="box">
        <canBeScrapped result="notARealItem" />
      </itemClass>
    )");
    user->giveItem(&server->getFirstItem());

    WHEN("he tries to scrap it") {
      client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

      // [Then the server doesn't crash]

      THEN("the user gets an error message") {
        CHECK(client->waitForMessage(ERROR_INVALID_ITEM));
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Scrapping with bad data", "[scrapping]") {
  SECTION("Empty slot") {
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));
    CHECK(client.waitForMessage(ERROR_EMPTY_SLOT));
  }

  SECTION("Invalid inventory slot") {
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 1000));
    CHECK(client.waitForMessage(ERROR_INVALID_SLOT));
  }

  SECTION("Invalid gear slot") {
    // Number of gear slots <= 9 < number of inventory slots
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Gear(), 9));
    CHECK(client.waitForMessage(ERROR_INVALID_SLOT));
  }

  SECTION("Invalid container serial") {
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(42, 0));
    CHECK(client.waitForMessage(WARNING_DOESNT_EXIST));
  }
}

// TODO later: unlock repair skill
