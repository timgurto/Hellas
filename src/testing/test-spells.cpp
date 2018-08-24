#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Non-talent spells") {
  GIVEN("a spell") {
    auto data = R"(
      <spell id="fireball" >
        <targets enemy=1 />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    CHECK_FALSE(user.getClass().knowsSpell("fireball"));

    WHEN("a player learns it") {
      user.getClass().teachSpell("fireball");

      THEN("he knows it") {
        CHECK(user.getClass().knowsSpell("fireball"));
        WAIT_UNTIL(c.knowsSpell("fireball"));

        AND_THEN("he doesn't know some other spell") {
          CHECK_FALSE(user.getClass().knowsSpell("iceball"));
          CHECK_FALSE(c.knowsSpell("iceball"));
        }
      }
    }
  }
}

TEST_CASE("Non-talent spells are persistent") {
  // Given a spell
  auto data = R"(
    <spell id="fireball" >
      <targets enemy=1 />
    </spell>
  )";
  {
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    // When Alice learns it
    user.getClass().teachSpell("fireball");

    // And the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    // Then she still knows it
    WAIT_UNTIL(user.getClass().knowsSpell("fireball"));
  }
}

TEST_CASE("Spell cooldowns", "[remote]") {
  GIVEN("Alice and Bob know a range of self-damaging spells") {
    auto data = R"(
      <spell id="hurtSelf1s" cooldown="1" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
      <spell id="hurtSelf" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);

    auto cAlice = TestClient::WithUsername("Alice");
    auto cBob = RemoteClient{"-username Bob"};
    s.waitForUsers(2);
    auto &alice = *s->getUserByName("Alice");
    auto &bob = *s->getUserByName("Bob");

    alice.getClass().teachSpell("hurtSelf");
    alice.getClass().teachSpell("hurtSelf1s");
    bob.getClass().teachSpell("hurtSelf1s");

    WHEN("Alice casts the spell with cooldown 1s") {
      cAlice.sendMessage(CL_CAST, "hurtSelf1s");
      REPEAT_FOR_MS(100);
      auto healthAfterFirstCast = alice.health();

      AND_WHEN("she tries casting it again") {
        cAlice.sendMessage(CL_CAST, "hurtSelf1s");

        THEN("she hasn't lost any health") {
          REPEAT_FOR_MS(100);
          CHECK(alice.health() >= healthAfterFirstCast);
        }
      }

      AND_WHEN("1s elapses") {
        REPEAT_FOR_MS(1000);

        AND_WHEN("she tries casting it again") {
          cAlice.sendMessage(CL_CAST, "hurtSelf1s");

          THEN("she has lost health") {
            REPEAT_FOR_MS(100);
            CHECK(alice.health() < healthAfterFirstCast);
          }
        }
      }

      AND_WHEN("she tries casting the spell with no cooldown") {
        cAlice.sendMessage(CL_CAST, "hurtSelf");

        THEN("she has lost health") {
          REPEAT_FOR_MS(100);
          CHECK(alice.health() < healthAfterFirstCast);
        }
      }

      AND_WHEN("Bob tries casting it") {
        auto healthBefore = bob.health();
        s.sendMessage(bob.socket(), TST_SEND_THIS_BACK,
                      makeArgs(CL_CAST, "hurtSelf1s"));

        THEN("he has lost health") {
          REPEAT_FOR_MS(100);
          CHECK(bob.health() < healthBefore);
        }
      }
    }
  }
}
