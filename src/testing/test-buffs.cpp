#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Buffs can be applied") {
  GIVEN("A buff") {
    auto data = R"(
      <buff id="intellect" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    WHEN("a user is given the buff") {
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.applyBuff(s.getFirstBuff(), user);

      THEN("he has the buff") { CHECK(user.buffs().size() == 1); }
    }
  }
}

TEST_CASE("Buffs disappear on death") {
  GIVEN("a dog and a flea buff") {
    auto data = R"(
      <buff id="flea" />
      <npcType id="dog" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    s.addNPC("dog", {10, 15});
    auto &dog = s.getFirstNPC();

    WHEN("the dog gets a flea buff and a flea debuff") {
      auto &flea = s.getFirstBuff();
      dog.applyBuff(flea, dog);
      dog.applyDebuff(flea, dog);

      CHECK(dog.debuffs().size() == 1);

      WAIT_UNTIL(c.objects().size() == 1);
      auto &cDog = c.getFirstNPC();
      WAIT_UNTIL(cDog.debuffs().size() == 1);

      WHEN("the dog dies") {
        dog.kill();

        THEN("it doesn't have any fleas") {
          CHECK(dog.buffs().empty());
          CHECK(dog.debuffs().empty());

          AND_THEN("and nearby users know it") {
            WAIT_UNTIL(cDog.debuffs().empty());
            WAIT_UNTIL(cDog.buffs().empty());
          }

          SECTION("buffs and debuffs can't be applied to a dead entity") {
            AND_WHEN("the flea buff tries to reapply to the dead dog") {
              dog.applyBuff(flea, dog);
              dog.applyDebuff(flea, dog);

              THEN("it has no buffs") {
                REPEAT_FOR_MS(100);
                CHECK(dog.buffs().empty());
                CHECK(dog.debuffs().empty());
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Interruptible buffs disappear on interrupt", "[combat]") {
  SECTION("When attacked") {
    GIVEN("An interruptible buff, and a fox") {
      useData(R"(
        <buff id="food" canBeInterrupted="1"/>
        <npcType id="fox" attack="1" attackTime="1" />
      )");

      server->addNPC("fox", {10, 15});

      WHEN("the user near the fox has the buff") {
        const auto &buff = server->getFirstBuff();
        user->applyBuff(buff, *user);
        CHECK(user->buffs().size() == 1);

        THEN("he loses the buff") { WAIT_UNTIL(user->buffs().empty()); }
      }
    }
  }

  GIVEN("An interruptible buff, and a recipe") {
    useData(R"(
        <buff id="food" canBeInterrupted="1"/>
        <recipe id="idea" time="100" />
        <item id="idea" />
      )");

    AND_GIVEN("the user has the buff") {
      const auto &buff = server->getFirstBuff();
      user->applyBuff(buff, *user);

      WHEN("he starts crafting") {
        client->sendMessage(CL_CRAFT, "idea");

        THEN("he loses the buff") { WAIT_UNTIL(user->buffs().empty()); }
      }

      WHEN("he starts moving") {
        client->sendMessage(CL_MOVE_TO, makeArgs(100, 100));

        THEN("he loses the buff") { WAIT_UNTIL(user->buffs().empty()); }
      }
    }
  }
}

TEST_CASE("Non-interruptible buffs persist when attacked", "[combat]") {
  GIVEN("A buff, and a fox") {
    auto data = R"(
      <buff id="intellect" />
      <npcType id="fox" attack="1" attackTime="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addNPC("fox", {10, 15});

    WHEN("a user near the fox has the buff") {
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.applyBuff(s.getFirstBuff(), user);
      CHECK(user.buffs().size() == 1);

      THEN("he loses the buff") {
        REPEAT_FOR_MS(100);
        CHECK(user.buffs().size() == 1);
      }
    }
  }
}

TEST_CASE("A buff that ends when out of energy") {
  GIVEN("A user with a cancel-on-OOE buff") {
    auto data = R"(
      <buff id="focus" cancelsOnOOE="1" />
      <buff id="drainEnergy" >
        <stats eps="-10000" />
      </buff>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    auto focus = s->findBuff("focus");
    user.applyBuff(*focus, user);
    CHECK(user.buffs().size() == 1);

    WHEN("the user loses a small amount of energy") {
      user.reduceEnergy(1);

      THEN("he still has a buff") { CHECK(user.buffs().size() == 1); }
    }

    WHEN("the user has no energy") {
      user.reduceEnergy(user.energy());

      THEN("he has no buffs") { CHECK(user.buffs().empty()); }
    }

    WHEN("he has a negative regen buff") {
      auto drainEnergy = s->findBuff("drainEnergy");
      user.applyBuff(*drainEnergy, user);
      CHECK(user.buffs().size() == 2);

      THEN("he has no buffs") { WAIT_UNTIL(user.buffs().size() == 1); }
    }
  }
}

TEST_CASE("A buff that changes allowed terrain") {
  GIVEN("a map with grass and water, and buff that allows water walking") {
    auto data = R"(
      <terrain index="G" id="grass" />
      <terrain index="." id="water" />
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <list id="all" >
          <allow id="grass" />
          <allow id="water" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="4" y="4" />
      <row    y="0" terrain = "GG.." />
      <row    y="1" terrain = "GG.." />
      <row    y="2" terrain = "...." />
      <row    y="3" terrain = "...." />

      <buff id="levitating" >
          <changeAllowedTerrain terrainList="all" />
      </buff>
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("the player has the buff") {
      auto levitating = s.getFirstBuff();
      user.applyBuff(levitating, user);

      THEN("he knows that the new terrain list is active") {
        WAIT_UNTIL(c.allowedTerrain() == "all");

        AND_THEN("he can walk to the other end of the map") {
          REPEAT_FOR_MS(2000) {
            c.sendMessage(CL_MOVE_TO, makeArgs(70, 10));
            SDL_Delay(5);
          }
          CHECK(user.location().x == 70.0);
        }
      }
    }
  }
}

TEST_CASE("A buff on new players") {
  GIVEN("a buff set to be given to new players") {
    auto data = R"(
      <buff id="newbie" onNewPlayers="1" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a user logs in") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      THEN("he has a buff") { CHECK(user.buffs().size() == 1); }
    }

    WHEN("Alice logs in, loses the buff, and logs out") {
      {
        auto c = TestClient::WithUsernameAndDataString("alice", data);
        s.waitForUsers(1);
        auto &user = s.getFirstUser();

        user.removeBuff("newbie");
        CHECK(user.buffs().empty());
      }

      AND_WHEN("Alice logs back in") {
        auto c = TestClient::WithUsernameAndDataString("alice", data);
        s.waitForUsers(1);
        auto &user = s.getFirstUser();

        THEN("she doesn't have any buffs") { CHECK(user.buffs().empty()); }
      }
    }
  }

  GIVEN("a buff") {
    auto data = R"(
      <buff id="godMode" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a user logs in") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      THEN("he has no buffs") { CHECK(user.buffs().empty()); }
    }
  }

  GIVEN("two buffs set to be given to new players") {
    auto data = R"(
      <buff id="blessing" onNewPlayers="1" />
      <buff id="curse" onNewPlayers="1" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a user logs in") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      THEN("he has two buff") { CHECK(user.buffs().size() == 2); }
    }
  }
}

TEST_CASE("Buff removal propagates to client") {
  GIVEN("a buff that lasts 1s") {
    auto data = R"(
      <buff id="sneezy" duration="1" />
      <npcType id="cat" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto &sneezy = s.getFirstBuff();

    WHEN("a user gets the buff") {
      user.applyBuff(sneezy, user);

      AND_WHEN("the buff expires") {
        WAIT_UNTIL(user.buffs().empty());

        THEN("he knows he has no buffs") {
          REPEAT_FOR_MS(100);
          CHECK(c->character().buffs().empty());
        }
      }
    }

    WHEN("an NPC gets the buff") {
      s.addNPC("cat", {10, 15});
      auto &cat = s.getFirstNPC();
      cat.applyBuff(sneezy, cat);

      AND_WHEN("the buff expires") {
        WAIT_UNTIL(cat.buffs().empty());

        THEN("the user knows that has it no buffs") {
          REPEAT_FOR_MS(100);
          const auto &cCat = c.getFirstNPC();
          CHECK(cCat.buffs().empty());
        }
      }
    }
  }
}

TEST_CASE("Returning users know their buffs") {
  // Given a user with a buff
  auto data = R"(
      <buff id="grumpy" />
    )";
  auto s = TestServer::WithDataString(data);
  auto username = ""s;
  {
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    username = c.name();
    auto &user = s.getFirstUser();
    user.applyBuff(s.getFirstBuff(), user);

    // When he logs out then back in
  }
  auto c = TestClient::WithUsernameAndDataString(username, data);
  s.waitForUsers(1);

  // Then he knows he still has the buff.
  WAIT_UNTIL(c->character().buffs().size() == 1);
}

TEST_CASE_METHOD(ServerAndClientWithData, "Object-granted buffs") {
  GIVEN("a user owns a buff-granting object") {
    useData(R"(
      <buff id="glowing" />
      <objectType id="uranium">
        <grantsBuff id="glowing" radius="5" />
      </objectType>
    )");
    server->addObject("uranium", {20, 20}, user->name());

    THEN("he has no buffs") { CHECK(user->buffs().empty()); }

    WHEN("he touches the object") {
      while (user->location() != MapPoint{20, 20}) {
        client->sendMessage(CL_MOVE_TO, makeArgs(20, 20));
        REPEAT_FOR_MS(100);
      }

      THEN("he has a buff") { CHECK(user->buffs().size() == 1); }

      AND_WHEN("he moves away from it") {
        while (user->location() != MapPoint{10, 10}) {
          client->sendMessage(CL_MOVE_TO, makeArgs(10, 10));
          REPEAT_FOR_MS(100);
        }

        THEN("he has no buffs") { CHECK(user->buffs().empty()); }
      }
    }
  }
}

TEST_CASE("Buffs that don't stack") {
  GIVEN(
      "Two non-stacking paint colours, and a nonStacking emotion "
      "category") {
    auto data = R"(
      <buff id="paintedRed" >
        <nonStacking category="paint" />
      </buff>
      <buff id="paintedBlue" >
        <nonStacking category="paint" />
      </buff>
      <buff id="happy" >
        <nonStacking category="emotion" />
      </buff>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto &paintedRed = s.findBuff("paintedRed");
    const auto &paintedBlue = s.findBuff("paintedBlue");
    const auto &happy = s.findBuff("happy");

    WHEN("both paints are applied to a user") {
      user.applyBuff(paintedRed, user);
      user.applyBuff(paintedBlue, user);

      THEN("the user has one buff") { CHECK(user.buffs().size() == 1); }
    }

    WHEN("a paint and an emotion are applied to a user") {
      user.applyBuff(paintedRed, user);
      user.applyBuff(happy, user);

      THEN("the user has two buffs") { CHECK(user.buffs().size() == 2); }
    }
  }
}
