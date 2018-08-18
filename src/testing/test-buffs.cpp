#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Buffs can be applied") {
  GIVEN("A buff") {
    auto data = R"(
      <buff id="intellect" duration="10" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    WHEN("a user near is given the buff") {
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
      <buff id="food" duration="10" canBeInterrupted="1"/>
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
