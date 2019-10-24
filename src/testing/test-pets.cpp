#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Taming NPCs") {
  GIVEN("a cat") {
    auto data = R"(
      <npcType id="cat" maxHealth="1" />
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

      AND_GIVEN("Alice logs in") {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);
        s.waitForUsers(1);

        AND_GIVEN("another dog") {
          s.addNPC("dog", {15, 10});

          THEN("Alice's dog loses health") {
            WAIT_UNTIL(dog.health() < dog.stats().maxHealth);

            AND_THEN("she receives a SV_ENTITY_HIT_ENTITY message") {
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

    AND_GIVEN("it's owned by a city") {
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
    }
  }
}
