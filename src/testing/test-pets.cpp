#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Taming NPCs") {
  GIVEN("a tamable cat") {
    auto data = R"(
      <npcType id="cat" maxHealth="1" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    s.addNPC("cat");
    const auto &cat = s.getFirstNPC();

    THEN("it has no owner") {
      CHECK(cat.owner().type == Permissions::Owner::NONE);

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
          CHECK(cat.owner().type == Permissions::Owner::NONE);
        }
      }
    }
  }
}

TEST_CASE("Bad arguments to taming command") {
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

TEST_CASE("Taming an NPC untargets it") {
  GIVEN("a tamable NPC with plenty of health") {
    auto data = R"(
      <npcType id="hippo" maxHealth="10000" >
        <canBeTamed />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    s.addNPC("hippo");
    const auto &hippo = s.getFirstNPC();

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

TEST_CASE("Pet shares owner's diplomacy", "[ai][war]") {
  GIVEN("an aggressive dog NPC") {
    auto data = R"(
      <npcType id="dog" maxHealth="1000" attack="2" speed="1" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("dog", {10, 15});
    auto &dog = s.getFirstNPC();

    WHEN("it becomes owned by a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      const auto &user = s.getFirstUser();
      dog.permissions().setPlayerOwner(user.name());

      THEN("the owner doesn't lose any health") {
        REPEAT_FOR_MS(1000) {
          REQUIRE(user.health() == user.stats().maxHealth);
        }
      }

      AND_WHEN("the owner tries to target it") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

        THEN("the dog doesn't lose any health") {
          REPEAT_FOR_MS(100);
          REQUIRE(dog.health() == dog.stats().maxHealth);
        }
      }
    }

    AND_GIVEN("it's owned by an offline player, Alice") {
      dog.permissions().setPlayerOwner("Alice");

      AND_GIVEN("a player named Bob") {
        auto c = TestClient::WithUsernameAndDataString("Bob", data);
        s.waitForUsers(1);
        const auto &bob = s.getFirstUser();

        WHEN("Bob tries to target Alice's dog") {
          c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

          THEN("the dog doesn't lose any health") {
            REPEAT_FOR_MS(100);
            REQUIRE(dog.health() == dog.stats().maxHealth);
          }
        }

        WHEN("Bob is at war with Alice") {
          s.wars().declare({"Alice", Belligerent::PLAYER},
                           {"Bob", Belligerent::PLAYER});

          THEN("Bob loses health") {
            WAIT_UNTIL(bob.health() < bob.stats().maxHealth);

            AND_WHEN("Bob tries to target Alice's dog") {
              c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

              THEN("the dog loses health") {
                WAIT_UNTIL(dog.health() < dog.stats().maxHealth);
              }
            }
          }
        }

        AND_GIVEN("Bob is in a city") {
          s.cities().createCity("Athens");
          s.cities().addPlayerToCity(bob, "Athens");

          AND_GIVEN("the city is at war with Alice") {
            s.wars().declare({"Alice", Belligerent::PLAYER},
                             {"Athens", Belligerent::CITY});
            CHECK(s.wars().isAtWar({"Alice", Belligerent::PLAYER},
                                   {"Bob", Belligerent::PLAYER}));

            WHEN("Bob tries to target Alice's dog") {
              c.sendMessage(CL_TARGET_ENTITY, makeArgs(dog.serial()));

              THEN("the dog loses health") {
                WAIT_UNTIL(dog.health() < dog.stats().maxHealth);
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
            WAIT_UNTIL(dog.health() < dog.stats().maxHealth);

            AND_THEN("the observer receives a SV_ENTITY_HIT_ENTITY message") {
              CHECK(c.waitForMessage(SV_ENTITY_HIT_ENTITY));
            }
          }
        }
      }

      AND_GIVEN("another dog owned by offline player, Bob") {
        s.addNPC("dog", {15, 10});
        auto *dog2 = dynamic_cast<NPC *>(s.entities().find(dog.serial() + 1));
        CHECK(dog2 != nullptr);
        dog2->permissions().setPlayerOwner("Bob");

        AND_WHEN("some time passes") {
          REPEAT_FOR_MS(100);

          THEN("Alice's dog hasn't lost any health") {
            CHECK(dog.health() == dog.stats().maxHealth);
          }
        }

        AND_WHEN("Alice and Bob declare war") {
          s.wars().declare({"Alice", Belligerent::PLAYER},
                           {"Bob", Belligerent::PLAYER});

          THEN("Alice's dog loses health") {
            WAIT_UNTIL(dog.health() < dog.stats().maxHealth);
          }
        }
      }
    }

    AND_GIVEN("it's owned by the city of Athens") {
      s.cities().createCity("Athens");
      dog.permissions().setCityOwner("Athens");

      AND_GIVEN("a player named Bob") {
        auto c = TestClient::WithUsernameAndDataString("Bob", data);
        s.waitForUsers(1);
        const auto &bob = s.getFirstUser();

        WHEN("Bob is at war with the city") {
          s.wars().declare({"Athens", Belligerent::CITY},
                           {"Bob", Belligerent::PLAYER});

          THEN("Bob loses health") {
            WAIT_UNTIL(bob.health() < bob.stats().maxHealth);
          }
        }
      }

      AND_GIVEN("another dog owned by the city of Sparta") {
        s.addNPC("dog", {15, 10});
        auto *dog2 = dynamic_cast<NPC *>(s.entities().find(dog.serial() + 1));
        CHECK(dog2 != nullptr);
        s.cities().createCity("Sparta");
        dog2->permissions().setCityOwner("Sparta");

        AND_WHEN("Athens and Sparta declare war") {
          s.wars().declare({"Athens", Belligerent::CITY},
                           {"Sparta", Belligerent::CITY});

          THEN("Athens' dog loses health") {
            WAIT_UNTIL(dog.health() < dog.stats().maxHealth);
          }
        }
      }
    }
  }
}

TEST_CASE("Pets follow their owners") {
  GIVEN("A guinea pig") {
    auto data = R"(
      <npcType id="guineaPig" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("guineaPig", {10, 15});
    auto &guineaPig = s.getFirstNPC();

    AND_GIVEN("It's owned by a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      guineaPig.permissions().setPlayerOwner(user.name());

      WHEN("The player moves away") {
        user.teleportTo({100, 100});

        THEN("The guinea pig moves nearby") {
          const auto maxDist = 30.0;
          const auto timeAllowed = ms_t{10000};
          WAIT_UNTIL_TIMEOUT(
              distance(guineaPig.location(), user.location()) <= maxDist,
              timeAllowed);
        }
      }
    }
  }
}

TEST_CASE("Non-tamable NPCs can't be tamed") {
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
        CHECK(tiger.owner().type == Permissions::Owner::NONE);
      }
    }
  }
}

TEST_CASE("Taming can require an item") {
  GIVEN("An NPC that can be tamed with an item") {
    auto data = R"(
      <item id="chocolate" />
      <item id="stinkBug" />
      <npcType id="girl" maxHealth="1" >
        <canBeTamed consumes="chocolate" />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);

    const auto *chocolate = &s.findItem("chocolate");
    const auto *stinkBug = &s.findItem("stinkBug");

    auto &girl = s.addNPC("girl", {10, 15});

    AND_GIVEN("a player") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      WHEN("he tries to tame it") {
        c.sendMessage(CL_TAME_NPC, makeArgs(girl.serial()));

        THEN("he receives a warning") {
          CHECK(c.waitForMessage(WARNING_ITEM_NEEDED));

          AND_THEN("it is still unowned") {
            CHECK(girl.owner().type == Permissions::Owner::NONE);
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
            CHECK(girl.owner().type == Permissions::Owner::NONE);
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

TEST_CASE("Pets can be slaughtered") {
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
    pig.permissions().setPlayerOwner(user.name());

    WHEN("he tries to slaughter it") {
      c.sendMessage(CL_DEMOLISH, makeArgs(pig.serial()));

      THEN("it dies") { WAIT_UNTIL(pig.isDead()); }
    }
  }
}

TEST_CASE("Neutral pets defend their owners") {
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
      pet.permissions().setPlayerOwner(c->username());

      THEN("the pet attacks the hostile NPC") {
        WAIT_UNTIL(pet.target() == &hostile);
      }
    }
  }
}

TEST_CASE("Neutral pets have the correct UI colours") {
  GIVEN("a neutral NPC") {
    auto data = R"(
      <npcType id="dog" maxHealth="1" isNeutral="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto &dog = s.addNPC("dog", {10, 15});

    AND_GIVEN("a city and a citizen") {
      auto c = TestClient::WithDataString(data);
      s.cities().createCity("Athens");

      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      s.cities().addPlayerToCity(user, "Athens");

      WAIT_UNTIL(c.objects().size() == 1);
      const auto &cDog = c.getFirstNPC();

      WHEN("the NPC is owned by the player") {
        dog.permissions().setPlayerOwner(user.name());

        THEN("he sees it as 'self' coloured") {
          WAIT_UNTIL(cDog.nameColor() == Color::COMBATANT_SELF);
        }
      }
    }
  }
}
