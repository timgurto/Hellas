#include "TestClient.h"
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

TEST_CASE("Interruptible buffs disappear on interrupt", "[combat]") {
  GIVEN("An interruptible buff, and a fox") {
    auto data = R"(
      <buff id="food" canBeInterrupted="1"/>
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

      THEN("he loses the buff") { WAIT_UNTIL(user.buffs().empty()); }
    }
  }
}

TEST_CASE("Normal buffs persist when attacked", "[combat]") {
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
