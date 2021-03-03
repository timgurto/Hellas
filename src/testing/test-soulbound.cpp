#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Soulbound items are deleted when dropped") {
  GIVEN("rings binds on pickup") {
    useData(R"(
      <item id="ring" bind="pickup" />
    )");

    THEN("the user knows that rings bind on pickup") {
      CHECK(client->items().begin()->second.bindsOnPickup());
    }

    WHEN("a user receives a ring") {
      auto &ring = server->getFirstItem();
      user->giveItem(&ring);

      AND_WHEN("he tries to drop it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("it is gone from his inventory") {
          WAIT_UNTIL(!user->inventory(0).first.hasItem());

          AND_THEN("no dropped item was created") {
            REPEAT_FOR_MS(100);
            CHECK(server->entities().empty());
          }
        }
      }

      THEN("he knows it's soulbound") {
        const auto &clientItem = client->inventory().at(0).first;
        WAIT_UNTIL(clientItem.type());

        CHECK(clientItem.isSoulbound());
      }
    }
  }

  GIVEN("rings bind on equip") {
    useData(R"(
      <item id="ring" bind="equip" gearSlot="1" />
    )");

    WHEN("a user receives a ring") {
      auto &ring = server->getFirstItem();
      user->giveItem(&ring);

      AND_WHEN("he tries to drop it") {
        client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("it is no longer equipped") {
          WAIT_UNTIL(!user->gear(1).first.hasItem());
        }
      }

      AND_WHEN("he equips it") {
        client->sendMessage(
            CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0, Serial::Gear(), 1));
        WAIT_UNTIL(user->gear(1).first.hasItem());

        AND_WHEN("he tries to drop it") {
          client->sendMessage(CL_DROP, makeArgs(Serial::Gear(), 1));

          THEN("it is gone from his inventory") {
            WAIT_UNTIL(!user->inventory(0).first.hasItem());

            AND_THEN("no dropped item was created") {
              REPEAT_FOR_MS(100);
              CHECK(server->entities().empty());
            }
          }
        }

        AND_WHEN("he unequips it") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Gear(), 1,
                                                      Serial::Inventory(), 0));

          AND_WHEN("he tries to drop it") {
            client->sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

            THEN("it is gone from his inventory") {
              WAIT_UNTIL(!user->inventory(0).first.hasItem());

              AND_THEN("no dropped item was created") {
                REPEAT_FOR_MS(100);
                CHECK(server->entities().empty());
              }
            }
          }
        }

        AND_WHEN("he receives another ring in his inventory") {
          user->giveItem(&ring);

          AND_WHEN("he tries to swap from gear to inventory") {
            client->sendMessage(
                CL_SWAP_ITEMS,
                makeArgs(Serial::Gear(), 1, Serial::Inventory(), 0));

            THEN("the newly equipped ring is soulbound") {
              REPEAT_FOR_MS(100);
              CHECK(user->gear(1).first.isSoulbound());
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "By default, items do not bind") {
  GIVEN("a user has a plain item") {
    useData(R"(
      <item id="rock" />
    )");
    user->giveItem(&server->getFirstItem());

    THEN("he knows it isn't soulbound") {
      CHECK_FALSE(client->inventory().at(0).first.isSoulbound());
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Soulbound items can be stored only in private containers") {
  GIVEN("BoP apples, non-binding oranges, barrels") {
    useData(R"(
      <item id="apple" bind="pickup" />
      <item id="orange" />
      <objectType id="barrel" >
        <container slots="1" />
      </objectType>
    )");

    AND_GIVEN("a user has an apple") {
      const auto *apple = &server->findItem("apple");
      const auto *orange = &server->findItem("orange");
      user->giveItem(apple);

      AND_GIVEN("a publicly owned barrel") {
        auto &barrel = server->addObject("barrel", {15, 15});

        WHEN("he tries to put it into the barrel") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      barrel.serial(), 0));

          THEN("he still has it") {
            REPEAT_FOR_MS(100);
            CHECK(user->inventory(0).first.hasItem());
          }

          THEN("he receives a warning") {
            CHECK(client->waitForMessage(WARNING_OBJECT_MUST_BE_PRIVATE));
          }
        }

        AND_GIVEN("the barrel has an orange") {
          barrel.container().addItems(orange);

          WHEN("the user tries to swap the orange with his apple") {
            client->sendMessage(
                CL_SWAP_ITEMS,
                makeArgs(barrel.serial(), 0, Serial::Inventory(), 0));

            THEN("he still has the apple") {
              REPEAT_FOR_MS(100);
              CHECK(user->inventory(0).first.type() == apple);
            }

            THEN("he receives a warning") {
              CHECK(client->waitForMessage(WARNING_OBJECT_MUST_BE_PRIVATE));
            }
          }
        }
      }

      AND_GIVEN("he owns a barrel") {
        auto &barrel = server->addObject("barrel", {15, 15}, user->name());

        WHEN("he tries to put it into the barrel") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      barrel.serial(), 0));

          THEN("he no longer has it") {
            WAIT_UNTIL(!user->inventory(0).first.hasItem());
          }
        }

        AND_GIVEN("the barrel has an orange") {
          barrel.container().addItems(orange);

          WHEN("the user tries to swap the orange with his apple") {
            client->sendMessage(
                CL_SWAP_ITEMS,
                makeArgs(barrel.serial(), 0, Serial::Inventory(), 0));

            THEN("he has an orange") {
              WAIT_UNTIL(user->inventory(0).first.type() == orange);
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Soulbound status is persistent") {
  GIVEN("hats are BoE") {
    auto data = R"(
      <item id="hat" gearSlot="0" bind="equip" />
    )";
    auto s = TestServer::WithDataString(data);

    SECTION("Inventory") {
      // AND GIVEN Alice logs in
      {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();

        // AND GIVEN she has a soulbound hat and a non-soulbound hat
        alice.giveItem(&s.getFirstItem(), 2);
        alice.inventory(0).first.onEquip();

        // WHEN she logs off and back on
      }
      {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();

        // THEN her first hat is still soulbound
        CHECK(alice.inventory(0).first.isSoulbound());

        // AND_THEN her second hat is not
        CHECK_FALSE(alice.inventory(1).first.isSoulbound());
      }
    }

    SECTION("Gear") {
      // AND GIVEN Alice logs in
      {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();

        // AND GIVEN she has a soulbound hat equipped
        alice.giveItem(&s.getFirstItem());
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

        // WHEN she logs off and back on
      }
      {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();

        // THEN her hat is still soulbound
        CHECK(alice.gear(0).first.isSoulbound());
      }
    }
  }

  GIVEN("BoE shoes, and a shoebox object type") {
    auto data = R"(
      <item id="shoes" bind="equip" />
      <objectType id="shoebox">
        <container slots="2"/>
      </objectType>
    )";
    {
      auto s = TestServer::WithDataString(data);

      // AND GIVEN Alice has two shoes
      auto c = TestClient::WithUsernameAndDataString("Alice", data);
      s.waitForUsers(1);
      auto &alice = s.getFirstUser();
      alice.giveItem(&s.getFirstItem(), 2);

      // AND GIVEN one is soulbound
      alice.inventory(0).first.onEquip();

      // AND GIVEN she owns a shoebox
      const auto &shoebox = s.addObject("shoebox", {20, 20}, "Alice");

      // AND GIVEN the shoes are in the shoebox
      c.sendMessage(CL_SWAP_ITEMS,
                    makeArgs(Serial::Inventory(), 0, shoebox.serial(), 0));
      c.sendMessage(CL_SWAP_ITEMS,
                    makeArgs(Serial::Inventory(), 1, shoebox.serial(), 1));
      WAIT_UNTIL(shoebox.container().at(1).first.hasItem());

      // WHEN the server restarts
    }
    {
      auto s = TestServer::WithDataStringAndKeepingOldData(data);

      // THEN the first shoe is still soulbound
      const auto &shoebox = s.getFirstObject();
      CHECK(shoebox.container().at(0).first.isSoulbound());

      // AND THEN the second shoe is still not soulbound
      CHECK_FALSE(shoebox.container().at(1).first.isSoulbound());
    }
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "Soulbound items can't be traded") {
  GIVEN("A marble store, and blue and red BoE marbles") {
    useData(R"(
      <item id="blueMarble" bind="equip" />
      <item id="redMarble" bind="equip" />
      <objectType id="marbleStore" merchantSlots="1">
        <container slots="1"/>
      </objectType>
    )");
    const auto *blueMarble = &server->findItem("blueMarble");
    const auto *redMarble = &server->findItem("redMarble");

    AND_GIVEN("Alice owns a marble store selling blue for red") {
      auto &store = server->addObject("marbleStore", {20, 20}, "Alice");
      cAlice->sendMessage(
          CL_SET_MERCHANT_SLOT,
          makeArgs(store.serial(), 0, "blueMarble", 1, "redMarble", 1));

      AND_GIVEN("Bob has a red marble (price)") {
        uBob->giveItem(redMarble);

        AND_GIVEN("the store has a blue marble (ware)") {
          uAlice->giveItem(blueMarble);
          cAlice->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      store.serial(), 0));
          WAIT_UNTIL(store.container().at(0).first.hasItem());

          AND_GIVEN("the blue marble (ware) is soulbound") {
            store.container().at(0).first.onEquip();

            WHEN("Bob tries to buy the blue marble") {
              cBob->sendMessage(CL_TRADE, makeArgs(store.serial(), 0));

              THEN("he still has his red one") {
                REPEAT_FOR_MS(100);
                CHECK(uBob->inventory(0).first.type() == redMarble);
              }

              THEN("he gets a warning") {
                CHECK(cBob->waitForMessage(WARNING_WARE_IS_SOULBOUND));
              }
            }
          }

          AND_GIVEN("the red marble price) is soulbound") {
            uBob->inventory(0).first.onEquip();

            WHEN("Bob tries to buy the blue marble") {
              cBob->sendMessage(CL_TRADE, makeArgs(store.serial(), 0));

              THEN("he gets a warning") {
                CHECK(cBob->waitForMessage(WARNING_PRICE_IS_SOULBOUND));
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Containers with soulbound items can't be ceded") {
  GIVEN("carton containers and BoP egg items") {
    useData(R"(
      <item id="egg" bind="pickup" />
      <objectType id="carton">
        <container slots="1"/>
      </objectType>
    )");

    AND_GIVEN("the user is in a city") {
      server->cities().createCity("Athens", {}, user->name());
      server->cities().addPlayerToCity(*user, "Athens");

      AND_GIVEN("he owns a carton") {
        auto &carton = server->addObject("carton", {10, 10}, user->name());

        AND_GIVEN("the carton contains a [soulbound] egg") {
          const auto *egg = &server->getFirstItem();
          carton.container().addItems(egg);

          WHEN("he tries to cede the carton to her city") {
            client->sendMessage(CL_CEDE, makeArgs(carton.serial()));

            THEN("it still belongs to him") {
              REPEAT_FOR_MS(100);
              CHECK(carton.permissions.isOwnedByPlayer(user->name()));
            }

            THEN("he receives a warning") {
              CHECK(client->waitForMessage(WARNING_CONTAINS_BOUND_ITEM));
            }
          }
        }
      }
    }
  }
}
