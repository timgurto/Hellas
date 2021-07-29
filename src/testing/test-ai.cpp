#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "NPCs chain pull", "[ai]") {
  GIVEN("a user with a spear") {
    useData(R"(
      <npcType id="bear" />
      <npcType id="critter"  isNeutral="1" />
      <item id="spear" gearSlot="6">
        <weapon range="25" consumes="spear" damage="1" speed="1" />
      </item>
    )");

    auto spear = &server->getFirstItem();
    user->giveItem(spear);
    client->sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Inventory(), 0, Serial::Gear(), 6));
    WAIT_UNTIL(user->gear(6).first.type() == spear);

    WHEN(
        "there are two bears and a neutral critter, close to each other but "
        "out of aggro range") {
      auto &bear1 = server->addNPC("bear", {100, 5});
      auto &bear2 = server->addNPC("bear", {100, 15});
      auto &critter = server->addNPC("critter", {100, 25});

      THEN("the two close bears are unaware of him") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(bear1.isAwareOf(*user));
        CHECK_FALSE(bear2.isAwareOf(*user));
      }

      AND_WHEN("There's a third bear off in the distance") {
        auto &bear3 = server->addNPC("bear", {10, 300});

        AND_WHEN("the user throws a spear at one") {
          WAIT_UNTIL(client->objects().size() >= 2);
          client->sendMessage(CL_TARGET_ENTITY, makeArgs(bear1.serial()));

          THEN("the two close bears are aware of him") {
            WAIT_UNTIL(bear1.isAwareOf(*user));
            WAIT_UNTIL(bear2.isAwareOf(*user));

            AND_THEN("the distant bear and critter are not") {
              CHECK_FALSE(bear3.isAwareOf(*user));
              CHECK_FALSE(critter.isAwareOf(*user));
            }
          }
        }
      }
    }
  }
}

TEST_CASE("NPCs don't attack each other", "[ai][combat]") {
  GIVEN("An aggressive wolf NPC type") {
    auto data = R"(
      <npcType id="wolf" maxHealth="10000" attack="2" speed="10" />
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("Two wolves") {
      s.addNPC("wolf", {10, 15});
      s.addNPC("wolf", {15, 10});

      WHEN("Enough time passes for a few hits") {
        REPEAT_FOR_MS(50);

        THEN("the first hasn't lost any health") {
          const auto &wolf1 = s.getFirstNPC();
          CHECK(wolf1.health() == wolf1.stats().maxHealth);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Pathfinding", "[ai]") {
  const auto LARGE_MAP = R"(
    <newPlayerSpawn x="10" y="10" range="0" />
    <terrain index="." id="grass" />
    <list id="default" default="1" >
      <allow id="grass" />
    </list>
    <size x="100" y="100" />
    <row y= "0" terrain = "...................................................................................................." />
    <row y= "1" terrain = "...................................................................................................." />
    <row y= "2" terrain = "...................................................................................................." />
    <row y= "3" terrain = "...................................................................................................." />
    <row y= "4" terrain = "...................................................................................................." />
    <row y= "5" terrain = "...................................................................................................." />
    <row y= "6" terrain = "...................................................................................................." />
    <row y= "7" terrain = "...................................................................................................." />
    <row y= "8" terrain = "...................................................................................................." />
    <row y= "9" terrain = "...................................................................................................." />
    <row y="10" terrain = "...................................................................................................." />
    <row y="11" terrain = "...................................................................................................." />
    <row y="12" terrain = "...................................................................................................." />
    <row y="13" terrain = "...................................................................................................." />
    <row y="14" terrain = "...................................................................................................." />
    <row y="15" terrain = "...................................................................................................." />
    <row y="16" terrain = "...................................................................................................." />
    <row y="17" terrain = "...................................................................................................." />
    <row y="18" terrain = "...................................................................................................." />
    <row y="19" terrain = "...................................................................................................." />
    <row y="20" terrain = "...................................................................................................." />
    <row y="21" terrain = "...................................................................................................." />
    <row y="22" terrain = "...................................................................................................." />
    <row y="23" terrain = "...................................................................................................." />
    <row y="24" terrain = "...................................................................................................." />
    <row y="25" terrain = "...................................................................................................." />
    <row y="26" terrain = "...................................................................................................." />
    <row y="27" terrain = "...................................................................................................." />
    <row y="28" terrain = "...................................................................................................." />
    <row y="29" terrain = "...................................................................................................." />
    <row y="30" terrain = "...................................................................................................." />
    <row y="31" terrain = "...................................................................................................." />
    <row y="32" terrain = "...................................................................................................." />
    <row y="33" terrain = "...................................................................................................." />
    <row y="34" terrain = "...................................................................................................." />
    <row y="35" terrain = "...................................................................................................." />
    <row y="36" terrain = "...................................................................................................." />
    <row y="37" terrain = "...................................................................................................." />
    <row y="38" terrain = "...................................................................................................." />
    <row y="39" terrain = "...................................................................................................." />
    <row y="40" terrain = "...................................................................................................." />
    <row y="41" terrain = "...................................................................................................." />
    <row y="42" terrain = "...................................................................................................." />
    <row y="43" terrain = "...................................................................................................." />
    <row y="44" terrain = "...................................................................................................." />
    <row y="45" terrain = "...................................................................................................." />
    <row y="46" terrain = "...................................................................................................." />
    <row y="47" terrain = "...................................................................................................." />
    <row y="48" terrain = "...................................................................................................." />
    <row y="49" terrain = "...................................................................................................." />
    <row y="50" terrain = "...................................................................................................." />
    <row y="51" terrain = "...................................................................................................." />
    <row y="52" terrain = "...................................................................................................." />
    <row y="53" terrain = "...................................................................................................." />
    <row y="54" terrain = "...................................................................................................." />
    <row y="55" terrain = "...................................................................................................." />
    <row y="56" terrain = "...................................................................................................." />
    <row y="57" terrain = "...................................................................................................." />
    <row y="58" terrain = "...................................................................................................." />
    <row y="59" terrain = "...................................................................................................." />
    <row y="60" terrain = "...................................................................................................." />
    <row y="61" terrain = "...................................................................................................." />
    <row y="62" terrain = "...................................................................................................." />
    <row y="63" terrain = "...................................................................................................." />
    <row y="64" terrain = "...................................................................................................." />
    <row y="65" terrain = "...................................................................................................." />
    <row y="66" terrain = "...................................................................................................." />
    <row y="67" terrain = "...................................................................................................." />
    <row y="68" terrain = "...................................................................................................." />
    <row y="69" terrain = "...................................................................................................." />
    <row y="70" terrain = "...................................................................................................." />
    <row y="71" terrain = "...................................................................................................." />
    <row y="72" terrain = "...................................................................................................." />
    <row y="73" terrain = "...................................................................................................." />
    <row y="74" terrain = "...................................................................................................." />
    <row y="75" terrain = "...................................................................................................." />
    <row y="76" terrain = "...................................................................................................." />
    <row y="77" terrain = "...................................................................................................." />
    <row y="78" terrain = "...................................................................................................." />
    <row y="79" terrain = "...................................................................................................." />
    <row y="80" terrain = "...................................................................................................." />
    <row y="81" terrain = "...................................................................................................." />
    <row y="82" terrain = "...................................................................................................." />
    <row y="83" terrain = "...................................................................................................." />
    <row y="84" terrain = "...................................................................................................." />
    <row y="85" terrain = "...................................................................................................." />
    <row y="86" terrain = "...................................................................................................." />
    <row y="87" terrain = "...................................................................................................." />
    <row y="88" terrain = "...................................................................................................." />
    <row y="89" terrain = "...................................................................................................." />
    <row y="90" terrain = "...................................................................................................." />
    <row y="91" terrain = "...................................................................................................." />
    <row y="92" terrain = "...................................................................................................." />
    <row y="93" terrain = "...................................................................................................." />
    <row y="94" terrain = "...................................................................................................." />
    <row y="95" terrain = "...................................................................................................." />
    <row y="96" terrain = "...................................................................................................." />
    <row y="97" terrain = "...................................................................................................." />
    <row y="98" terrain = "...................................................................................................." />
    <row y="99" terrain = "...................................................................................................." />
  )";

  GIVEN("a colliding square wall object type, and a wolf NPC") {
    auto data = ""s;
    data = R"(
      <npcType id="wolf" maxHealth="10000" attack="1" speed="100"
        pursuesEndlessly="1" >
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </npcType>
      <objectType id="wall">
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </objectType>
    )";

    SECTION("Single static obstacle") {
      GIVEN("a wall between a wolf and a player") {
        useData(data.c_str());
        server->addObject("wall", {60, 10});
        auto &wolf = server->addNPC("wolf", {90, 10});

        WHEN("the wolf starts chasing the user") {
          wolf.makeAwareOf(*user);

          THEN("it can reach him") {
            WAIT_UNTIL_TIMEOUT(distance(wolf, *user) <= wolf.attackRange(),
                               10000);
          }
        }
      }
    }

    SECTION("Random obstacles") {
      GIVEN("three walls randomly between a wolf and the user") {
        data += R"(
            <spawnPoint y="55" x="55" type="wall" quantity="3" radius="50" />
          )";
        useData(data.c_str());

        auto &wolf = server->addNPC("wolf", {100, 100});

        WHEN("the wolf starts chasing the user") {
          wolf.makeAwareOf(*user);

          THEN("it can reach him") {
            WAIT_UNTIL_TIMEOUT(distance(wolf, *user) <= wolf.attackRange(),
                               10000);
          }
        }
      }
    }

    SECTION("Reacting to a path becoming blocked") {
      GIVEN("a clear path between wolf and user") {
        useData(data.c_str());
        auto &wolf = server->addNPC("wolf", {280, 10});

        WHEN("the wolf starts following a path to the user") {
          const auto wolfStartLocation = wolf.location();
          wolf.makeAwareOf(*user);
          WAIT_UNTIL(wolf.location() != wolfStartLocation);

          AND_WHEN("a wall appears that blocks the wolf") {
            server->addObject("wall", {200, 10});

            THEN("the wolf can reach the user without getting stuck") {
              WAIT_UNTIL_TIMEOUT(distance(wolf, *user) <= wolf.attackRange(),
                                 3000);
            }
          }
        }
      }
    }

    SECTION("Reacting to the target moving") {
      GIVEN("a clear path between wolf and user") {
        useData(data.c_str());
        auto &wolf = server->addNPC("wolf", {280, 10});

        WHEN("the wolf starts following a path to the user") {
          const auto wolfStartLocation = wolf.location();
          wolf.makeAwareOf(*user);
          WAIT_UNTIL(wolf.location() != wolfStartLocation);

          AND_WHEN("the user teleports away") {
            user->teleportTo({200, 200});

            THEN("the wolf can reach the user") {
              WAIT_UNTIL_TIMEOUT(distance(wolf, *user) <= wolf.attackRange(),
                                 10000);
            }
          }
        }
      }
    }

    SECTION("Performance on a large, empty map") {
      GIVEN("a very large map") {
        data += LARGE_MAP;
        useData(data.c_str());

        AND_GIVEN("a wolf that's very far from the player") {
          auto &wolf = server->addNPC("wolf", {2000, 2000});

          WHEN("the wolf starts chasing the user") {
            wolf.makeAwareOf(*user);

            THEN("it begins moving in a reasonable amount of time") {
              const auto originalLocation = wolf.location();
              REPEAT_FOR_MS(500);
              CHECK(wolf.isAwareOf(*user));
              CHECK(wolf.location() != originalLocation);
            }
          }
        }
      }
    }

    GIVEN("a map spanning a number of collision chunks") {
      data += R"(
        <newPlayerSpawn x="10" y="10" range="0" />
        <terrain index="." id="grass" />
        <list id="default" default="1" >
          <allow id="grass" />
        </list>
        <size x="30" y="2" />
        <row y= "0" terrain = ".............................." />
        <row y= "1" terrain = ".............................." />
      )";
      useData(data.c_str());

      AND_GIVEN("the user is blocked by walls in the leftmost chunk") {
        /* ...W....|.......|.......|...
           .U.W....|.......|.......|.N.
           ...W....|.......|.......|... */
        server->addObject("wall", {150, 5});
        server->addObject("wall", {150, 15});
        server->addObject("wall", {150, 25});
        server->addObject("wall", {150, 35});
        server->addObject("wall", {150, 45});
        server->addObject("wall", {150, 55});
        server->addObject("wall", {150, 65});

        SECTION("Obstacles are properly identified across collision chunks") {
          AND_GIVEN("a wolf on the opposite side of the map") {
            const auto wolfStart = MapPoint{900, 30};
            auto &wolf = server->addNPC("wolf", wolfStart);

            WHEN("the wolf tries to chase the user") {
              wolf.makeAwareOf(*user);

              THEN("the wolf never moves (i.e., it knows there's no path") {
                REPEAT_FOR_MS(1000);
                CHECK(wolf.location() == wolfStart);
              }
            }
          }

          SECTION("spawned mobs return if target becomes unreachable") {
            AND_GIVEN("a wolf spawns near the user") {
              const auto wolfStart = MapPoint{120, 55};
              auto spawner = Spawner{wolfStart, &server->getFirstNPCType()};
              spawner.spawn();
              WAIT_UNTIL(server->entities().size() == 8);  // 7 walls + wolf
              auto &wolf = server->getFirstNPC();

              AND_GIVEN("it is chasing him") {
                wolf.makeAwareOf(*user);
                WAIT_UNTIL(wolf.location() != wolfStart);

                WHEN("the user teleports very far away") {
                  user->teleportTo({900, 30});

                  THEN("the wolf returns home") {
                    WAIT_UNTIL(wolf.location() == wolfStart);
                  }
                }
              }
            }
          }
        }
      }
    }

    SECTION("More efficient than breadth-first") {
      GIVEN("a very large map") {
        data += LARGE_MAP;
        useData(data.c_str());

        AND_GIVEN("the user is walled off to block the direct-path shortcut") {
          /* ...W.
             .U.W.
             ...W.
             ...W.
             .....
             WWWW.
             ..... */
          server->addObject("wall", {90, 0});
          server->addObject("wall", {90, 10});
          server->addObject("wall", {90, 20});
          server->addObject("wall", {90, 30});
          server->addObject("wall", {0, 90});
          server->addObject("wall", {10, 90});
          server->addObject("wall", {20, 90});
          server->addObject("wall", {30, 90});
          server->addObject("wall", {40, 90});
          server->addObject("wall", {50, 90});
          server->addObject("wall", {60, 90});
          server->addObject("wall", {70, 90});
          server->addObject("wall", {80, 90});
          server->addObject("wall", {90, 90});

          AND_GIVEN("a wolf on the opposite side of the map from the user") {
            auto &wolf = server->addNPC("wolf", {2000, 2000});

            WHEN("the wolf starts chasing the user") {
              const auto originalLocation = wolf.location();
              wolf.makeAwareOf(*user);

              THEN("it begins moving in a reasonable amount of time") {
                WAIT_UNTIL_TIMEOUT(wolf.location() != originalLocation, 5000);
              }
            }
          }
        }
      }
    }

    SECTION("a large target entity doesn't block pathfinding to it") {
      GIVEN("dinosaurs have a wide collision rect") {
        data += R"(
          <npcType id="dinosaur">
            <collisionRect x="-100" y="-100" w="200" h="200" />
          </npcType>
        )";
        useData(data.c_str());

        AND_GIVEN("a dinosaur and a wolf hostile to it") {
          auto &dinosaur = server->addNPC("dinosaur", {100, 150});
          dinosaur.permissions.setPlayerOwner(user->name());
          dinosaur.ai.giveOrder(AI::PetOrder::ORDER_TO_STAY);

          auto &wolf = server->addNPC("wolf", {275, 150});

          WHEN("the wolf starts chasing the dinosaur") {
            const auto originalLocation = wolf.location();
            wolf.makeAwareOf(dinosaur);

            THEN("it begins moving (i.e., a path is successfully found)") {
              WAIT_UNTIL_TIMEOUT(wolf.location() != originalLocation, 5000);
            }
          }
        }
      }
    }
  }

  GIVEN("walls, and ranged longbowmen") {
    auto data = R"(
      <npcType id="longbowman" maxHealth="10000" attack="1" isRanged="1"
        pursuesEndlessly="1" >
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </npcType>
      <objectType id="wall">
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </objectType>
    )";
    useData(data);

    AND_GIVEN("the user is blocked off from a longbowman") {
      auto &longbowman = server->addNPC("longbowman", {280, 280});
      server->addObject("wall", {90, 0});
      server->addObject("wall", {90, 10});
      server->addObject("wall", {90, 20});
      server->addObject("wall", {90, 30});
      server->addObject("wall", {90, 40});
      server->addObject("wall", {90, 50});
      server->addObject("wall", {90, 60});
      server->addObject("wall", {90, 70});
      server->addObject("wall", {90, 80});
      server->addObject("wall", {0, 90});
      server->addObject("wall", {10, 90});
      server->addObject("wall", {20, 90});
      server->addObject("wall", {30, 90});
      server->addObject("wall", {40, 90});
      server->addObject("wall", {50, 90});
      server->addObject("wall", {60, 90});
      server->addObject("wall", {70, 90});
      server->addObject("wall", {80, 90});
      server->addObject("wall", {90, 90});

      SECTION("longer-ranged NPCs don't need to path as close") {
        WHEN("the longbowman starts chasing the player") {
          longbowman.makeAwareOf(*user);

          THEN("the user takes damage)") {
            WAIT_UNTIL_TIMEOUT(user->isMissingHealth(), 10000);
          }
        }
      }

      SECTION("ranged pets follow close") {
        WHEN("the longbowman becomes the user's pet [and tries to follow]") {
          longbowman.permissions.setPlayerOwner(user->name());

          THEN("it doesn't move (i.e., correctly concludes there's no path)") {
            const auto oldLocation = longbowman.location();
            REPEAT_FOR_MS(1000);
            CHECK(longbowman.location() == oldLocation);
          }
        }
      }
    }
  }

  SECTION("Path lengths are limited") {
    GIVEN("kobold NPCs (without pursuesEndlessly)") {
      auto data = ""s;
      data = R"(
        <npcType id="kobold" attack="1" />
      )";

      AND_GIVEN("a very large map") {
        data += LARGE_MAP;
        useData(data.c_str());

        AND_GIVEN("a kobold on the other side of the map from the user") {
          auto &kobold = server->addNPC("kobold", {2000, 2000});

          WHEN("the kobold starts chasing the user") {
            const auto originalLocation = kobold.location();
            kobold.makeAwareOf(*user);

            THEN("it never moves") {
              REPEAT_FOR_MS(5000);
              CHECK(kobold.location() == originalLocation);
            }
          }
        }
      }
    }
  }
}
