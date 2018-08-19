#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Spells learned outside of talents") {
  auto s = TestServer{};
  auto c = TestClient{};

  WHEN("a player learns a spell") {
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.getClass().teachSpell("fireball");
  }
}
