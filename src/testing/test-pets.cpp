#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Taming NPCs", "[pets]") {
  GIVEN("a tamable cat") {
    auto data = R"(
      <npcType id="cat" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &cat = s.addNPC("cat");
    cat.reduceHealth(9999);

    THEN("it has no owner") {
      CHECK(cat.owner().type == Permissions::Owner::MOB);

      WHEN("a user tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial()));

        THEN("it belongs to the user") {
          WAIT_UNTIL(cat.owner().type == Permissions::Owner::PLAYER);
          CHECK(cat.owner().name == c->username());
        }
      }

      WHEN("a user tries to tame it with too many arguments") {
        c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial(), 42));

        THEN("it has no owner") {
          REPEAT_FOR_MS(50);
          CHECK(cat.owner().type == Permissions::Owner::MOB);
        }
      }
    }
  }
}

TEST_CASE("Owned NPCs can't be tamed", "[pets][permissions]") {
  GIVEN("a tameable NPC") {
    auto data = R"(
      <npcType id="cat" maxHealth="1" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    auto &cat = s.addNPC("cat");

    AND_GIVEN("it's owned by an offline player, Alice") {
      cat.permissions.setPlayerOwner("Alice");

      WHEN("another player tries to tame it") {
        s.waitForUsers(1);
        c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial()));

        THEN("it's still owned by Alice") {
          REPEAT_FOR_MS(100);
          CHECK(cat.permissions.isOwnedByPlayer("Alice"));
        }
      }
    }
  }
}

TEST_CASE("Bad arguments to taming command", "[pets]") {
  GIVEN("a server and client") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    WHEN("the client tries to tame a nonexistent NPC") {
      c.sendMessage(CL_TAME_NPC, makeArgs(42));

      THEN("the client receives a warning") {
        CHECK(c.waitForMessage(WARNING_DOESNT_EXIST));
      }
    }
  }
}

TEST_CASE("Taming an NPC untargets it", "[pets]") {
  GIVEN("a tamable NPC with plenty of health") {
    auto data = R"(
      <npcType id="hippo" maxHealth="100000000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &hippo = s.addNPC("hippo");
    hippo.reduceHealth(99990000);

    AND_GIVEN("a user is attacking it") {
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(hippo.serial()));
      const auto &user = s.getFirstUser();
      WAIT_UNTIL(user.action() == User::ATTACK);

      WHEN("he tames it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(hippo.serial()));

        THEN("he is no longer attacking it") {
          WAIT_UNTIL(user.action() != User::ATTACK);
        }
      }
    }
  }
}

TEST_CASE("Pet shares owner's diplomacy",
          "[ai][war][permissions][pets][combat]") {
  GIVEN("an aggressive dog NPC") {
    auto data = R"(
      <npcType id="dog" maxHealth="1000" attack="2" speed="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &dog = s.addNPC("dog", {10, 15});

    WHEN("it becomes owned by a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      const auto &user = s.getFirstUser();
      dog.permissions.setPlayerOwner(user.name());

      THEN("the owner doesn't lose any health") {
        const auto healthBefore = user.health();
        REPEAT_FOR_MS(100) { REQUIRE(user.health() == healthBefore); }
      }

      AND_WHEN("the owner tries to target it") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

        THEN("the dog doesn't lose any health") {
          const auto healthBefore = dog.health();
          REPEAT_FOR_MS(100) { REQUIRE(dog.health() == healthBefore); }
        }
      }
    }

    AND_GIVEN("it's owned by an offline player, Alice") {
      dog.permissions.setPlayerOwner("Alice");

      AND_GIVEN("a player named Bob") {
        auto c = TestClient::WithUsernameAndDataString("Bob", data);
        s.waitForUsers(1);
        const auto &bob = s.getFirstUser();

        WHEN("Bob tries to target Alice's dog") {
          c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

          THEN("the dog doesn't lose any health") {
            const auto healthBefore = dog.health();
            REPEAT_FOR_MS(100) { REQUIRE(dog.health() == healthBefore); }
          }
        }

        WHEN("Bob is at war with Alice") {
          s.wars().declare({"Alice", Belligerent::PLAYER},
                           {"Bob", Belligerent::PLAYER});

          THEN("Bob loses health") {
            WAIT_UNTIL(bob.isMissingHealth());

            AND_WHEN("Bob tries to target Alice's dog") {
              c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

              THEN("the dog loses health") {
                WAIT_UNTIL(dog.isMissingHealth());
              }
            }
          }
        }

        AND_GIVEN("Bob is in a city") {
          s.cities().createCity("Athens", {}, {});
          s.cities().addPlayerToCity(bob, "Athens");

          AND_GIVEN("the city is at war with Alice") {
            s.wars().declare({"Alice", Belligerent::PLAYER},
                             {"Athens", Belligerent::CITY});
            CHECK(s.wars().isAtWar({"Alice", Belligerent::PLAYER},
                                   {"Bob", Belligerent::PLAYER}));

            WHEN("Bob tries to target Alice's dog") {
              c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

              THEN("the dog loses health") {
                WAIT_UNTIL(dog.isMissingHealth());
              }
            }
          }
        }
      }

      AND_GIVEN("An observer user logs in, far away from the action") {
        auto c = TestClient::WithDataString(data);
        s.waitForUsers(1);
        auto &user = s.getFirstUser();
        user.teleportTo({200, 200});

        AND_GIVEN("another dog") {
          s.addNPC("dog", {15, 10});

          THEN("Alice's dog loses health") {
            WAIT_UNTIL(dog.isMissingHealth());

            AND_THEN("the observer receives a SV_ENTITY_HIT_ENTITY message") {
              CHECK(c.waitForMessage(SV_ENTITY_HIT_ENTITY));
            }
          }
        }
      }

      AND_GIVEN("another dog owned by offline player, Bob") {
        auto &dog2 = s.addNPC("dog", {15, 10});
        dog2.permissions.setPlayerOwner("Bob");

        THEN("Alice's dog hasn't lost any health over time") {
          const auto healthBefore = dog.health();
          REPEAT_FOR_MS(100) { REQUIRE(dog.health() == healthBefore); }
        }

        AND_WHEN("Alice and Bob declare war") {
          s.wars().declare({"Alice", Belligerent::PLAYER},
                           {"Bob", Belligerent::PLAYER});

          THEN("Alice's dog loses health") {
            WAIT_UNTIL(dog.isMissingHealth());
          }
        }
      }
    }

    AND_GIVEN("it's owned by the city of Athens") {
      s.cities().createCity("Athens", {}, {});
      dog.permissions.setCityOwner("Athens");

      AND_GIVEN("a player named Bob") {
        auto c = TestClient::WithUsernameAndDataString("Bob", data);
        s.waitForUsers(1);
        const auto &bob = s.getFirstUser();

        WHEN("Bob is at war with the city") {
          s.wars().declare({"Athens", Belligerent::CITY},
                           {"Bob", Belligerent::PLAYER});

          THEN("Bob loses health") { WAIT_UNTIL(bob.isMissingHealth()); }
        }
      }

      AND_GIVEN("another dog owned by the city of Sparta") {
        auto &dog2 = s.addNPC("dog", {15, 10});
        s.cities().createCity("Sparta", {}, {});
        dog2.permissions.setCityOwner("Sparta");

        AND_WHEN("Athens and Sparta declare war") {
          s.wars().declare({"Athens", Belligerent::CITY},
                           {"Sparta", Belligerent::CITY});

          THEN("Athens' dog loses health") {
            WAIT_UNTIL(dog.isMissingHealth());
          }
        }
      }
    }
  }
}

TEST_CASE("Pets follow their owners", "[pets][ai]") {
  GIVEN("A guinea pig") {
    auto data = R"(
      <npcType id="guineaPig" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &guineaPig = s.addNPC("guineaPig", {10, 15});

    AND_GIVEN("It's owned by a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      guineaPig.permissions.setPlayerOwner(user.name());

      WHEN("The player moves away") {
        user.teleportTo({100, 100});

        THEN("The guinea pig moves nearby") {
          const auto maxDist = AI::FOLLOW_DISTANCE;
          const auto timeAllowed = ms_t{10000};
          WAIT_UNTIL_TIMEOUT(distance(guineaPig, user) <= maxDist, timeAllowed);
        }
      }
    }
  }
}

TEST_CASE("Non-tamable NPCs can't be tamed", "[pets]") {
  GIVEN("A non-tamable tiger NPC") {
    auto data = R"(
      <npcType id="tiger" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("tiger", {10, 15});
    auto &tiger = s.getFirstNPC();

    WHEN("a player tries to tame it") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      c.sendMessage(CL_TAME_NPC, makeArgs(tiger.serial()));

      THEN("it is still unowned") {
        REPEAT_FOR_MS(100);
        CHECK(tiger.owner().type == Permissions::Owner::MOB);
      }
    }
  }
}

TEST_CASE("Taming can require an item", "[pets]") {
  GIVEN("An NPC that can be tamed with an item") {
    auto data = R"(
      <item id="chocolate" />
      <item id="stinkBug" />
      <npcType id="girl" maxHealth="10000" >
        <canBeTamed consumes="chocolate" />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);

    const auto *chocolate = &s.findItem("chocolate");
    const auto *stinkBug = &s.findItem("stinkBug");

    auto &girl = s.addNPC("girl", {10, 15});
    girl.reduceHealth(9999);

    AND_GIVEN("a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      WHEN("he tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(girl.serial()));

        THEN("he receives a warning") {
          CHECK(c.waitForMessage(WARNING_ITEM_NEEDED));

          AND_THEN("it is still unowned") {
            CHECK(girl.owner().type == Permissions::Owner::MOB);
          }
        }
      }

      AND_GIVEN("he has the item") {
        user.giveItem(chocolate);

        WHEN("he tries to tame the NPC") {
          c.sendMessage(CL_TAME_NPC, makeArgs(girl.serial()));

          THEN("it belongs to him") {
            WAIT_UNTIL(girl.owner().type == Permissions::Owner::PLAYER);

            AND_THEN("he no longer has the item") {
              auto consumable = ItemSet{};
              consumable.add(chocolate);
              WAIT_UNTIL(!user.hasItems(consumable));
            }
          }
        }
      }

      AND_GIVEN("he has the wrong item") {
        user.giveItem(stinkBug);

        WHEN("he tries to tame the NPC") {
          c.sendMessage(CL_TAME_NPC, makeArgs(girl.serial()));

          THEN("it is still unowned") {
            REPEAT_FOR_MS(100);
            CHECK(girl.owner().type == Permissions::Owner::MOB);
          }
        }

        AND_GIVEN("he also has the right item, in the second inventory slot") {
          user.giveItem(chocolate);

          WHEN("he tries to tame the NPC") {
            c.sendMessage(CL_TAME_NPC, makeArgs(girl.serial()));

            THEN("it belongs to him") {
              WAIT_UNTIL(girl.owner().type == Permissions::Owner::PLAYER);

              AND_THEN("he still has the wrong item") {
                auto extraStuff = ItemSet{};
                extraStuff.add(stinkBug);
                REPEAT_FOR_MS(100);
                CHECK(user.hasItems(extraStuff));
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Pets can be slaughtered", "[pets]") {
  GIVEN("a player with a pet") {
    auto data = R"(
      <npcType id="pig" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("pig", {10, 15});
    auto &pig = s.getFirstNPC();

    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    pig.permissions.setPlayerOwner(user.name());

    WHEN("he tries to slaughter it") {
      c.sendMessage(CL_DESTROY_OBJECT, makeArgs(pig.serial()));

      THEN("it dies") { WAIT_UNTIL(pig.isDead()); }
    }
  }
}

TEST_CASE("Neutral pets defend their owners", "[pets][combat][ai]") {
  GIVEN("an NPC attacking a player") {
    auto data = R"(
      <npcType id="dog" maxHealth="1000" attack="2" speed="1" isNeutral="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    s.addNPC("dog", {10, 5});
    auto &hostile = s.getFirstNPC();
    hostile.makeAwareOf(user);
    WAIT_UNTIL(hostile.target() == &user);

    WHEN("the player is given a neutral pet") {
      auto &pet = s.addNPC("dog", {10, 15});
      pet.permissions.setPlayerOwner(c->username());

      THEN("the pet attacks the hostile NPC") {
        WAIT_UNTIL(pet.target() == &hostile);
      }
    }
  }
}

TEST_CASE("Pets don't attack neutral NPCs", "[pets][combat][ai]") {
  GIVEN("two neutral NPCs") {
    auto data = R"(
      <npcType id="dog" maxHealth="1000" attack="2" speed="1" isNeutral="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &rex = s.addNPC("dog", {10, 15});
    auto &fido = s.addNPC("dog", {10, 10});

    AND_GIVEN("one is a pet") {
      rex.permissions.setPlayerOwner("Alice");

      THEN("they don't fight") {
        REPEAT_FOR_MS(1000);
        CHECK(!fido.isMissingHealth());
      }
    }
  }
}

TEST_CASE("NPCs defend themselves against NPC attackers",
          "[pets][combat][ai]") {
  GIVEN("two neutral NPCs") {
    auto data = R"(
      <npcType id="dog" maxHealth="1000" attack="2" speed="1" isNeutral="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &rex = s.addNPC("dog", {10, 15});
    auto &fido = s.addNPC("dog", {10, 10});

    AND_GIVEN("one is a pet") {
      rex.permissions.setPlayerOwner("Alice");

      WHEN("the pet attacks the NPC") {
        rex.makeAwareOf(fido);

        THEN("it becomes aware of the pet") { WAIT_UNTIL(fido.isAwareOf(rex)); }
      }
    }
  }
}

TEST_CASE("Neutral pets have the correct UI colours", "[pets][ui]") {
  GIVEN("a neutral NPC") {
    auto data = R"(
      <npcType id="dog" maxHealth="1" isNeutral="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &dog = s.addNPC("dog", {10, 15});

    AND_GIVEN("a city and a citizen") {
      auto c = TestClient::WithDataString(data);
      s.cities().createCity("Athens", {}, {});

      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      s.cities().addPlayerToCity(user, "Athens");

      WAIT_UNTIL(c.objects().size() == 1);
      const auto &cDog = c.getFirstNPC();

      WHEN("the NPC is owned by the player") {
        dog.permissions.setPlayerOwner(user.name());

        THEN("he sees it as 'self' coloured") {
          WAIT_UNTIL(cDog.nameColor() == Color::COMBATANT_SELF);
        }
      }
    }
  }
}

TEST_CASE("Pets from spawn points", "[pets][spawning][persistence]") {
  // Given a spawn point with a tameable NPC
  auto data = R"(
      <spawnPoint y="10" x="10" type="sheep" quantity="1" radius="100" respawnTime="1" />
      <npcType id="sheep" maxHealth="10000" >
        <canBeTamed/>
      </npcType>
    )";
  {
    auto s = TestServer::WithDataString(data);

    // When Alice tames the NPC
    auto c = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    auto &sheep = s.getFirstNPC();
    sheep.reduceHealth(9999);
    c.sendMessage(CL_TAME_NPC, makeArgs(sheep.serial()));
    WAIT_UNTIL(sheep.permissions.isOwnedByPlayer(user.name()));
    REPEAT_FOR_MS(100);

    // Then there are two NPCs (the pet and the respawn)
    CHECK(s.entities().size() == 2);

    // And when the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);

    // Then Alice still owns an NPC
    auto aliceOwnsAnNPC = false;
    REPEAT_FOR_MS(100);
    for (auto *ent : s.entities()) {
      auto *npc = dynamic_cast<NPC *>(ent);
      if (npc->permissions.isOwnedByPlayer("Alice")) aliceOwnsAnNPC = true;
    }
    CHECK(aliceOwnsAnNPC);
  }
}

TEST_CASE("Respawning tamed NPCs", "[pets][spawning]") {
  GIVEN("a fast spawn point with a tameable NPC") {
    auto data = R"(
      <spawnPoint y="10" x="10" type="sheep" quantity="1" radius="100" respawnTime="1" />
      <npcType id="sheep" maxHealth="10000" >
        <canBeTamed/>
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);

    WHEN("a user tames it") {
      auto &sheep = s.getFirstNPC();
      sheep.reduceHealth(9999);
      c.sendMessage(CL_TAME_NPC, makeArgs(sheep.serial()));

      AND_WHEN("server data is saved a couple of times") {
        s.saveData();
        REPEAT_FOR_MS(100);
        s.saveData();
        REPEAT_FOR_MS(100);

        THEN("there are two NPCs (the pet and the respawn)") {
          CHECK(s.entities().size() == 2);
        }
      }
    }
  }
}

TEST_CASE("Chance to tame based on health", "[pets]") {
  GIVEN("a tameable NPC") {
    auto data = R"(
      <npcType id="cat" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    auto &cat = s.addNPC("cat");

    WHEN("a user tries to tame it") {
      s.waitForUsers(1);
      c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial()));

      THEN("it is still unowned") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(cat.permissions.hasOwner());
      }
    }

    WHEN("it has 51% health") {
      cat.reduceHealth(4999);

      AND_WHEN("a user tries to tame it") {
        s.waitForUsers(1);
        c.sendMessage(CL_TAME_NPC, makeArgs(cat.serial()));

        THEN("he receives a message") {
          CHECK(c.waitForMessage(SV_TAME_ATTEMPT_FAILED));

          AND_THEN("it is still unowned") {
            REPEAT_FOR_MS(100);
            CHECK_FALSE(cat.permissions.hasOwner());
          }
        }
      }
    }
  }

  SECTION("NPC::getTameChance()") {
    auto data = R"(
      <npcType id="cow" maxHealth="10" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &cow = s.addNPC("cow");

    auto tameChancePerHealth = std::map<Hitpoints, double>{
        {1, 0.8}, {2, 0.6}, {3, 0.4}, {4, 0.2}, {5, 0.0},
        {6, 0.0}, {7, 0.0}, {8, 0.0}, {9, 0.0}, {10, 0.0}};
    for (auto pair : tameChancePerHealth) {
      auto health = pair.first;
      cow.healBy(10);
      cow.reduceHealth(10 - health);

      auto exepctedChance = pair.second;
      CHECK(almostEquals(cow.getTameChance(), exepctedChance));
    }
  }
}

TEST_CASE("Follower limits", "[pets]") {
  GIVEN("a user owns an NPC") {
    auto data = R"(
      <npcType id="dog" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    auto &petDog = s.addNPC("dog", {10, 15});
    petDog.reduceHealth(petDog.health() - 1);
    s.waitForUsers(1);
    c.sendMessage(CL_TAME_NPC, makeArgs(petDog.serial()));
    WAIT_UNTIL(petDog.permissions.hasOwner());

    AND_GIVEN("another tameable NPC with high tame chance") {
      auto &wildDog = s.addNPC("dog", {15, 10});
      wildDog.reduceHealth(wildDog.health() - 1);

      WHEN("he tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(wildDog.serial()));

        THEN("it is owned but not following") {
          REPEAT_FOR_MS(10);
          CHECK(wildDog.permissions.hasOwner());
          CHECK(wildDog.ai.currentOrder() == AI::ORDER_TO_STAY);

          AND_WHEN("he orders it to follow") {
            c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(wildDog.serial()));

            THEN("it is still not following") {
              REPEAT_FOR_MS(100);
              CHECK(wildDog.ai.currentOrder() == AI::ORDER_TO_STAY);
            }
          }
        }
      }

      WHEN("his pet dies") {
        petDog.kill();

        AND_WHEN("he tries to tame the other") {
          c.sendMessage(CL_TAME_NPC, makeArgs(wildDog.serial()));

          THEN("it has an owner and is following") {
            WAIT_UNTIL(wildDog.permissions.hasOwner());
            CHECK(wildDog.ai.currentOrder() == AI::ORDER_TO_FOLLOW);
          }
        }
      }

      WHEN("the base follower-limit stat is 2") {
        auto oldStats = User::OBJECT_TYPE.baseStats();
        auto highFollowerLimitStats = oldStats;
        highFollowerLimitStats.followerLimit = 2;
        User::OBJECT_TYPE.baseStats(highFollowerLimitStats);
        auto &user = s.getFirstUser();
        user.updateStats();

        AND_WHEN("he tries to tame the other") {
          c.sendMessage(CL_TAME_NPC, makeArgs(wildDog.serial()));

          THEN("it has an owner and is following") {
            WAIT_UNTIL(wildDog.permissions.hasOwner());
            CHECK(wildDog.ai.currentOrder() == AI::ORDER_TO_FOLLOW);
          }
        }

        User::OBJECT_TYPE.baseStats(oldStats);
      }
    }
  }

  GIVEN("a user owns an object") {
    auto data = R"(
      <objectType id="house" />
      <npcType id="dog" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);
    s.addObject("house", {10, 15}, c->username());

    AND_GIVEN("a tameable NPC with high tame chance") {
      auto &wildDog = s.addNPC("dog", {15, 10});
      wildDog.reduceHealth(wildDog.health() - 1);

      WHEN("he tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(wildDog.serial()));

        THEN("it has an owner") { WAIT_UNTIL(wildDog.permissions.hasOwner()); }
      }
    }
  }
}

TEST_CASE("Failed taming attempts should consume item", "[pets]") {
  GIVEN("an NPC that can be tamed with an item, with 0% success chance") {
    auto data = R"(
      <item id="chocolate" />
      <npcType id="girl" maxHealth="1" >
        <canBeTamed consumes="chocolate" />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto &girl = s.addNPC("girl", {10, 15});

    AND_GIVEN("a user has the item") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      const auto &chocolate = s.getFirstItem();
      user.giveItem(&chocolate);

      WHEN("he tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(girl.serial()));

        THEN("he no longer has the item") {
          WAIT_UNTIL(!user.inventory(0).first.hasItem());
        }
      }
    }
  }
}

// Note that this feature is currently disabled.
TEST_CASE_METHOD(ServerAndClientWithData, "Ceding pets to the city",
                 "[pets][city][permissions]") {
  GIVEN("A tameable NPC") {
    useData(R"(
      <npcType id="dog" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )");

    auto &dog = server->addNPC("dog", {10, 15});
    dog.reduceHealth(dog.health() - 1);

    AND_GIVEN("the player is in a city") {
      server->cities().createCity("Athens", {0, 0}, {});
      server->cities().addPlayerToCity(*user, "Athens");

      WHEN("he tames the NPC") {
        client->sendMessage(CL_TAME_NPC, makeArgs(dog.serial()));
        WAIT_UNTIL(dog.permissions.hasOwner());

        AND_WHEN("he tries to cede it to his city") {
          client->sendMessage(CL_CEDE, makeArgs(dog.serial()));

          THEN("the server survives") { server->nop(); }
        }
      }
    }
  }
}

TEST_CASE("Pet orders", "[pets][ai]") {
  GIVEN("a user and a dog") {
    auto data = R"(
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &dog = s.addNPC("dog", {10, 15});

    AND_GIVEN("the dog is the user's pet") {
      dog.permissions.setPlayerOwner(c->username());

      c.sendMessage(CL_ORDER_PET_TO_STAY, makeArgs(dog.serial()));
      WAIT_UNTIL(dog.ai.currentOrder() == AI::ORDER_TO_STAY);

      c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(dog.serial()));
      WAIT_UNTIL(dog.ai.currentOrder() == AI::ORDER_TO_FOLLOW);
    }

    WHEN("he tries to order it to stay") {
      c.sendMessage(CL_ORDER_PET_TO_STAY, makeArgs(dog.serial()));

      THEN("it is still set to Follow") {
        REPEAT_FOR_MS(100);
        CHECK(dog.ai.currentOrder() == AI::ORDER_TO_FOLLOW);
      }
    }
  }
}

TEST_CASE("Order pet to stay", "[pets][ai]") {
  GIVEN("a user and an NPC type") {
    auto data = R"(
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    AND_GIVEN("the user has a pet") {
      auto &dog = s.addNPC("dog", {10, 15});
      dog.permissions.setPlayerOwner(c->username());

      WHEN("its owner sends a Stay order") {
        c.sendMessage(CL_ORDER_PET_TO_STAY, makeArgs(dog.serial()));
        auto originalLocation = dog.location();

        AND_WHEN("its owner walks away") {
          c.simulateKeypress(SDL_SCANCODE_D);
          REPEAT_FOR_MS(2000);

          THEN("it doesn't move") { CHECK(dog.location() == originalLocation); }
        }
      }
    }
  }

  GIVEN("a server and client") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    WHEN("the client tries to order a nonexistent NPC to stay") {
      c.sendMessage(CL_ORDER_PET_TO_STAY, makeArgs(42));

      THEN("the server survives") { s.nop(); }
    }
  }
}

TEST_CASE("Ordering a pet to stay makes room for another follower",
          "[pets][stats]") {
  GIVEN("two tameable NPCs") {
    auto data = R"(
      <npcType id="cat" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &cat1 = s.addNPC("cat", {15, 10});
    cat1.reduceHealth(9999);
    auto &cat2 = s.addNPC("cat", {10, 15});
    cat2.reduceHealth(9999);

    WHEN("a user tames one and orders it to stay") {
      c.sendMessage(CL_TAME_NPC, makeArgs(cat1.serial()));
      c.sendMessage(CL_ORDER_PET_TO_STAY, makeArgs(cat1.serial()));
      WAIT_UNTIL(cat1.ai.currentOrder() == AI::ORDER_TO_STAY);

      AND_WHEN("he tames the other") {
        c.sendMessage(CL_TAME_NPC, makeArgs(cat2.serial()));

        THEN("the new pet is following him") {
          REPEAT_FOR_MS(100);
          CHECK(cat2.ai.currentOrder() == AI::ORDER_TO_FOLLOW);
        }
      }
    }
  }
}

TEST_CASE("Follow orders contribute to follower limit", "[pets][stats]") {
  GIVEN("a user has two pets ordered to stay") {
    auto data = R"(
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &dog1 = s.addNPC("dog", {15, 10});
    dog1.permissions.setPlayerOwner(c->username());
    dog1.ai.giveOrder(AI::ORDER_TO_STAY);
    auto &dog2 = s.addNPC("dog", {10, 15});
    dog2.permissions.setPlayerOwner(c->username());
    dog2.ai.giveOrder(AI::ORDER_TO_STAY);

    const auto &user = s.getFirstUser();
    CHECK(user.followers.num() == 0);

    WHEN("he orders one to follow") {
      c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(dog1.serial()));
      REPEAT_FOR_MS(100);

      AND_WHEN("he orders the other to follow") {
        c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(dog2.serial()));

        THEN("the second is not following") {
          REPEAT_FOR_MS(100);
          CHECK(dog2.ai.currentOrder() == AI::ORDER_TO_STAY);
        }
      }
    }
  }
}

TEST_CASE("Pets stop following if owner is far away", "[pets]") {
  GIVEN("a user with a pet") {
    auto data = R"(
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &dog = s.addNPC("dog", {15, 10});
    dog.permissions.setPlayerOwner(c->username());

    WHEN("he teleports far away") {
      auto &user = s.getFirstUser();
      user.teleportTo({300, 300});

      THEN("the pet is no longer following") {
        WAIT_UNTIL(dog.ai.currentOrder() == AI::ORDER_TO_STAY);
      }
    }
  }
}

TEST_CASE("Persistence of pet orders", "[pets][persistence]") {
  auto data = R"(
      <npcType id="dog" />
    )";

  // For each order typr
  const auto orders =
      std::set<AI::PetOrder>{AI::ORDER_TO_STAY, AI::ORDER_TO_FOLLOW};
  for (const auto order : orders) {
    {
      // Given a pet with the given order
      auto s = TestServer::WithDataString(data);
      auto &dog = s.addNPC("dog", {10, 15});
      dog.permissions.setPlayerOwner("Alice");
      dog.includeInPersistentState();
      dog.ai.giveOrder(order);

      // When the server restarts
    }
    {
      auto s = TestServer::WithDataStringAndKeepingOldData(data);

      // Then the pet still has the same order
      auto &dog = s.getFirstNPC();
      CHECK(dog.ai.currentOrder() == order);
    }
  }
}

TEST_CASE("Followers can count only once", "[pets][stats]") {
  GIVEN("A user with two pets on Stay") {
    auto data = R"(
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &dog1 = s.addNPC("dog", {15, 10});
    auto &dog2 = s.addNPC("dog", {10, 15});
    dog1.permissions.setPlayerOwner(c->username());
    dog2.permissions.setPlayerOwner(c->username());
    dog1.ai.giveOrder(AI::ORDER_TO_STAY);
    dog2.ai.giveOrder(AI::ORDER_TO_STAY);

    AND_GIVEN("he can have 2 followers") {
      auto oldStats = User::OBJECT_TYPE.baseStats();
      auto newStats = oldStats;
      newStats.followerLimit = 2;
      User::OBJECT_TYPE.baseStats(newStats);
      auto &user = s.getFirstUser();
      user.updateStats();

      WHEN("he orders the first pet to follow, twice") {
        c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(dog1.serial()));
        c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(dog1.serial()));

        AND_WHEN("he orders the second pet to follow") {
          c.sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(dog2.serial()));

          THEN("the second pet is following") {
            WAIT_UNTIL(dog2.ai.currentOrder() == AI::ORDER_TO_FOLLOW);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "A staying pet dying doesn't affect follower count",
                 "[pets][stats]") {
  GIVEN("A user with two pets on Stay and one on Follow") {
    useData(R"(
      <npcType id="dog" />
    )");

    auto &follower = server->addNPC("dog", {15, 10});
    auto &stayer1 = server->addNPC("dog", {10, 15});
    auto &stayer2 = server->addNPC("dog", {20, 10});
    follower.permissions.setPlayerOwner(user->name());
    stayer1.permissions.setPlayerOwner(user->name());
    stayer2.permissions.setPlayerOwner(user->name());
    follower.ai.giveOrder(AI::ORDER_TO_STAY);
    stayer1.ai.giveOrder(AI::ORDER_TO_STAY);
    stayer2.ai.giveOrder(AI::ORDER_TO_STAY);
    client->sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(follower.serial()));
    WAIT_UNTIL(follower.ai.currentOrder() == AI::ORDER_TO_FOLLOW);

    WHEN("a stayer dies") {
      stayer1.kill();

      AND_WHEN("he orders the other stayer to follow") {
        client->sendMessage(CL_ORDER_PET_TO_FOLLOW, makeArgs(stayer2.serial()));

        THEN("the second pet is not following") {
          REPEAT_FOR_MS(100);
          CHECK(stayer2.ai.currentOrder() == AI::ORDER_TO_STAY);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Pets can be fed", "[pets]") {
  GIVEN("a user with a pet and some food") {
    useData(R"(
      <npcType id="dog" maxHealth="10" />
      <item id="steak"> <tag name="food"/> </item>
      <buff id="food" duration="10" canBeInterrupted="1">
          <stats hps="500" />
      </buff>
    )");

    const auto &steak = server->getFirstItem();
    user->giveItem(&steak);

    auto &dog = server->addNPC("dog", {15, 10});
    dog.permissions.setPlayerOwner((*client)->username());

    AND_GIVEN("its health is down 1") {
      dog.reduceHealth(1);

      WHEN("he tries to feed it") {  // Normal success case
        client->sendMessage(CL_FEED_PET, makeArgs(dog.serial()));

        THEN("it gets to full health") {
          WAIT_UNTIL(!dog.isMissingHealth());

          AND_THEN("he no longer has the food") {
            CHECK(!user->inventory(0).first.hasItem());
          }
        }
      }

      WHEN("he has no food") {
        user->removeItems("food", 1);

        WHEN("he tries to feed it") {
          client->sendMessage(CL_FEED_PET, makeArgs(dog.serial()));

          THEN("it doesn't get healed") {
            REPEAT_FOR_MS(1500);
            CHECK(dog.isMissingHealth());
          }
        }
      }
    }

    WHEN("he tries to feed it") {  // Pet is at full health
      client->sendMessage(CL_FEED_PET, makeArgs(dog.serial()));

      THEN("he still has his food") {
        REPEAT_FOR_MS(100);
        CHECK(user->inventory(0).first.hasItem());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Bad args to CL_FEED_PET", "[pets]") {
  WHEN("a user tries to feed a nonexistent pet") {
    client.sendMessage(CL_FEED_PET, "50");

    THEN("the server survives") { REPEAT_FOR_MS(100); }
  }
}

TEST_CASE("Pet death", "[pets]") {
  GIVEN("a pet dog owned by an offline player") {
    auto data = R"(
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &dog = s.addNPC("dog", {15, 10});
    dog.permissions.setPlayerOwner("Alice");

    WHEN("the pet dies") {
      dog.kill();

      THEN("the server survives") {
        REPEAT_FOR_MS(100);
        s.nop();
      }
    }
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "Pets are anchored when ordered to stay",
                 "[pets][ai]") {
  GIVEN("dogs have a chase distance of 50px") {
    useData(R"(
      <npcType id="dog" maxHealth="1000" attack="1" maxDistanceFromSpawner="50" />
    )");

    AND_GIVEN("a pet dog owned by Alice, created at {200,20}") {
      auto &dog = server->addNPC("dog", {200, 20});
      dog.permissions.setPlayerOwner("Alice");

      AND_GIVEN("it is at {20,20}") {
        dog.teleportTo({20, 20});

        AND_GIVEN("it is ordered to stay") {
          cAlice->sendMessage(CL_ORDER_PET_TO_STAY, makeArgs(dog.serial()));

          AND_GIVEN("Bob is near the dog's original spawn point") {
            uBob->teleportTo({220, 20});

            WHEN("the dog is chasing Bob") {
              server->wars().declare({"Alice", Belligerent::PLAYER},
                                     {"Bob", Belligerent::PLAYER});
              dog.makeAwareOf(*uBob);
              WAIT_UNTIL(distance(dog.location(), {20, 20}) > 5.0);

              THEN("it eventually returns to {20,20}") {
                WAIT_UNTIL(distance(dog.location(), {20, 20}) < 5.0);
              }
            }
          }
        }
      }
    }
  }
}

// TODO:
// If follow count is reduced, one randomly stays
// Followers inside vehicle
// Can't order someone else's pet
// Followers teleport with player
// Followers are left behind when player dies
