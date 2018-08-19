#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Spells learned outside of talents") {
  auto s = TestServer{};
  auto c = TestClient{};

  s.waitForUsers(1);
  auto &user = s.getFirstUser();

  CHECK_FALSE(user.getClass().knowsSpell("fireball"));

  WHEN("a player learns a spell") {
    user.getClass().teachSpell("fireball");

    THEN("he knows it") {
      CHECK(user.getClass().knowsSpell("fireball"));

      AND_THEN("he doesn't know some other spell") {
        CHECK_FALSE(user.getClass().knowsSpell("iceball"));
      }
    }
  }
}
