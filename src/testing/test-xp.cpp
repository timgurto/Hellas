#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Bonus XP", "[xp]") {
  GIVEN("an enemy worth 100XP") {
    useData(R"(
      <npcType id="kobold" maxHealth="1" />
    )");
    const auto &kobold = server->addNPC("kobold", {10, 15});

    GIVEN("a player has lots of bonus XP") {
      user->setBonusXP(1000);

      WHEN("he kills the enemy") {
        client->sendMessage(CL_TARGET_ENTITY, makeArgs(kobold.serial()));

        THEN("he has 200XP") { WAIT_UNTIL(user->xp() == 200); }
      }
    }

    SECTION("Partial bonus xp remaining") {
      GIVEN("a player has only 1 bonus XP") {
        user->setBonusXP(1);

        WHEN("he kills the enemy") {
          client->sendMessage(CL_TARGET_ENTITY, makeArgs(kobold.serial()));

          THEN("he has 101XP") { WAIT_UNTIL(user->xp() == 101); }

          SECTION("bonus xp gets used up") {
            AND_WHEN("he kills another one") {
              const auto &kobold2 = server->addNPC("kobold", {10, 15});
              client->sendMessage(CL_TARGET_ENTITY, makeArgs(kobold2.serial()));

              THEN("he has 201XP") { WAIT_UNTIL(user->xp() == 201); }
            }
          }
        }
      }
    }
  }

  SECTION("Quests don't award bonus XP; only kills") {
    GIVEN("the user is on a quest") {
      useData(R"(
        <objectType id="A" />
        <quest id="quest" startsAt="A" endsAt="A" />
      )");
      const auto &questgiver = server->addObject("A", {10, 15});
      client->sendMessage(CL_ACCEPT_QUEST,
                          makeArgs("quest", questgiver.serial()));

      AND_GIVEN("he has lots of bonus XP") {
        user->setBonusXP(1000);

        WHEN("he completes the quest") {
          CHECK(user->xp() == 0);
          client->sendMessage(CL_COMPLETE_QUEST,
                              makeArgs("quest", questgiver.serial()));

          THEN("he gets only the baseline xp for that quest") {
            const auto &quest = server->getFirstQuest();
            const auto baselineXP = quest.getXPFor(user->level());
            WAIT_UNTIL(user->xp() == baselineXP);
          }
        }
      }
    }
  }
}

TEST_CASE("Bonus XP is persistent", "[xp][persistence]") {
  for (auto n : std::vector<XP>{0, 50}) {
    // GIVEN Alice has n bonus XP
    {
      auto server = TestServer{};
      auto client = TestClient::WithUsername("Alice");
      server.waitForUsers(1);
      server.getFirstUser().setBonusXP(n);

      // WHEN the server restarts
    }
    {
      auto server = TestServer::KeepingOldData();
      auto client = TestClient::WithUsername("Alice");
      server.waitForUsers(1);

      // AND WHEN Alice gets 100xp from a kill
      auto &user = server.getFirstUser();
      user.addXP(100, User::XP_FROM_KILL);

      // THEN Alice has the correct amount of xp
      const auto expectedXP = 100 + n;
      CHECK(user.xp() == expectedXP);
    }
  }
}

// Assign it once per day
// Offline players
// Online players
// Client knows amount (set while offline)
// Client knows amount (set while online)
// Client knows when bonus xp is awarded
//
