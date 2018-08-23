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

TEST_CASE("Spell cooldowns") {
  GIVEN("A range of self-damaging spells") {
    auto data = R"(
      <spell id="hurtSelfCooldown" cooldown="1" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
      <spell id="hurtSelf" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    auto &user = s.getFirstUser();
    user.getClass().teachSpell("hurtSelfCooldown");

    WHEN("a user casts the spell with cooldown 1s") {
      c.sendMessage(CL_CAST, "hurtSelfCooldown");
      REPEAT_FOR_MS(100);
      auto healthAfterFirstCast = user.health();

      AND_WHEN("he tries casting it again") {
        c.sendMessage(CL_CAST, "hurtSelfCooldown");

        THEN("he hasn't lost any health") {
          REPEAT_FOR_MS(100);
          CHECK(user.health() >= healthAfterFirstCast);
        }
      }

      AND_WHEN("he tries casting the spell with no cooldown") {
        user.getClass().teachSpell("hurtSelf");
        c.sendMessage(CL_CAST, "hurtSelf");

        THEN("he has lost health") {
          REPEAT_FOR_MS(100);
          CHECK(user.health() < healthAfterFirstCast);
        }
      }
    }
  }
}
