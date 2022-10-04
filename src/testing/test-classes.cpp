#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Class can be specified in TestClients") {
  GIVEN("Two classes, Class1 and Class2") {
    auto data = R"(
      <class name="Class1" />
      <class name="Class2" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a client is created as Class2") {
      auto c = TestClient::WithClassAndDataString("Class2", data);
      s.waitForUsers(1);

      THEN("his class is 'Class2' according to the server") {
        const auto &user = s.getFirstUser();
        CHECK(user.getClass().type().id() == "Class2");
      }
    }
  }
}

TEST_CASE("A talent tier can require a tool", "[talents][tool]") {
  GIVEN(
      "a level-2 user, tagged objects, and talent tiers with various "
      "requirements") {
    auto data = R"(
      <objectType id="rpa" maxHealth="1000">
        <collisionRect x="0" y="0" w="1" h="1" />
        <tag name="medicalSchool"/>
      </objectType>
      <class name="Doctor">
          <tree name="Surgeon">
              <tier>
                  <requires />
                  <talent type="stats" name="Meditate"> <stats energy="1" /> </talent>
              </tier>
              <tier>
                  <requires tool="medicalSchool" />
                  <talent type="stats" name="Study"> <stats energy="1" /> </talent>
              </tier>
              <tier>
                  <requires tool="food" />
                  <talent type="stats" name="Eat"> <stats health="1" /> </talent>
              </tier>
          </tree>
      </class>
    )";
    auto s = TestServer::WithDataString(data);
    const auto &doctor = s.getFirstClass();
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.levelUp();

    WHEN("he tries to take the simple talent") {
      c.sendMessage(CL_CHOOSE_TALENT, "Meditate");

      THEN("he has it") {
        const auto *talent = doctor.findTalent("Meditate");
        WAIT_UNTIL(user.getClass().hasTalent(talent));
      }
    }

    WHEN("there's a medicalSchool object nearby") {
      s.addObject("rpa", {10, 15});

      AND_WHEN("he tries to take the talent requiring a medicalSchool") {
        c.sendMessage(CL_CHOOSE_TALENT, "Study");

        THEN("he has it") {
          const auto *talent = doctor.findTalent("Study");
          WAIT_UNTIL(user.getClass().hasTalent(talent));
        }
      }

      AND_WHEN("he tries to take the talent requiring a different tool") {
        c.sendMessage(CL_CHOOSE_TALENT, "Eat");
        REPEAT_FOR_MS(100);

        THEN("he doesn't have it") {
          const auto *talent = doctor.findTalent("Eat");
          CHECK_FALSE(user.getClass().hasTalent(talent));
        }
      }
    }

    WHEN("he tries to take the talent with a tool requirement") {
      c.sendMessage(CL_CHOOSE_TALENT, "Study");
      REPEAT_FOR_MS(100);

      THEN("he doesn't have it") {
        const auto *talent = doctor.findTalent("Study");
        CHECK_FALSE(user.getClass().hasTalent(talent));
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Players can't lose load-bearing talents on death",
                 "[talents][death]") {
  GIVEN("players must walk before they can run") {
    useData(R"(
      <class name="DefaultClass">
          <tree name="Movement">
              <tier>
                  <requires />
                  <talent type="stats" name="Walking"> <stats /> </talent>
              </tier>
              <tier>
                  <requires pointsInTree="1" />
                  <talent type="stats" name="Running"> <stats /> </talent>
              </tier>
          </tree>
      </class>
    )");
    const auto *walking = server->getFirstClass().findTalent("Walking");
    const auto *running = server->getFirstClass().findTalent("Running");

    AND_GIVEN("the player has a point in each") {
      user->getClass().takeTalent(walking);
      user->getClass().takeTalent(running);

      // For a large number of samples
      for (auto i = 0; i != 20; ++i) {
        // When he loses a random talent as if from death
        const auto talentLost = user->getClass().loseARandomLeafTalent();

        // Then it was running he forgot, not walking
        INFO("The correct talent was removed " << i << " times.");
        REQUIRE(talentLost == "Running");

        // Re-learn for the next iteration
        user->getClass().takeTalent(running);
      }
    }
  }
}

TEST_CASE("Free spells", "[spells]") {
  GIVEN("a spell, but no free spells on classes") {
    auto data = R"(
      <spell id="tackle" ><targets enemy=1 /></spell>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a new user connects") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);

      THEN("he doesn't know Tackle") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(c.knowsSpell("tackle"));

        AND_THEN("his hotbar is empty") {
          const auto &user = s.getFirstUser();
          CHECK(user.hotbar().empty());
        }
      }
    }
  }

  GIVEN("a Bulbasaur class gets Tackle for free") {
    auto data = R"(
      <spell id="tackle" ><targets enemy=1 /></spell>
      <class name="Bulbasaur" freeSpell="tackle" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a new Bulbasaur connects") {
      auto c = TestClient::WithDataString(data);

      THEN("he knows Tackle") {
        WAIT_UNTIL(c.knowsSpell("tackle"));
        s.waitForUsers(1);

        AND_THEN("he has Tackle on his hotbar") {
          const auto &user = s.getFirstUser();
          WAIT_UNTIL(user.hotbar().size() >= 2);
          WAIT_UNTIL((user.hotbar()[1].category == HOTBAR_SPELL));
          WAIT_UNTIL(user.hotbar()[1].id == "tackle");
        }
      }
    }
  }

  SECTION("Retroactive free spells for returning players") {
    // GIVEN a server where Charmanders get no free spell
    {
      auto data = R"(
        <class name="Charmander" />
      )";
      auto s = TestServer::WithDataString(data);

      // AND GIVEN a pre-existing Charmander on that server
      auto c = TestClient::WithUsernameAndDataString("Alice", data);
      s.waitForUsers(1);

      // AND GIVEN a server where Charmanders get Scratch for free
    }
    {
      auto data = R"(
        <spell id="scratch" ><targets enemy=1 /></spell>
        <class name="Charmander" freeSpell="scratch" />
      )";
      auto s = TestServer::WithDataStringAndKeepingOldData(data);

      // WHEN the pre-existing Charmander logs in
      {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);

        // THEN it knows Scratch
        WAIT_UNTIL(c.knowsSpell("scratch"));

        // And when that same Charmander logs in again
      }
      {
        auto c = TestClient::WithUsernameAndDataString("Alice", data);

        // THEN it doesn't get a new-spell notification
        CHECK_FALSE(c.waitForMessage(SV_LEARNED_SPELL));
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Bonus XP", "[xp]") {
  GIVEN("an enemy worth 100XP") {
    useData(R"(
      <npcType id="kobold" maxHealth="1" />
    )");
    const auto &kobold = server->addNPC("kobold", {10, 15});

    GIVEN("a player has lots of bonus XP") {
      user->setBonusXP(1000);

      WHEN("he kills the enemy") {
        CHECK_FALSE(kobold.isDead());
        client->sendMessage(CL_TARGET_ENTITY, makeArgs(kobold.serial()));

        THEN("he has 200XP") { WAIT_UNTIL(user->xp() == 200); }
      }
    }
  }
}

// Partial
// Persistent
// Client knows
// Assign it once per day
// Only from kills, not quests
