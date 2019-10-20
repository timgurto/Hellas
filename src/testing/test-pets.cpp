#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Taming NPCs") {
  CL_TAME_NPC;

  GIVEN("a cat") {
    auto data = R"(
      <npcType id="cat" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addNPC("cat");
    const auto &cat = s.getFirstNPC();

    THEN("it has no owner") {
      auto owner = cat.owner();
      CHECK(owner.type == Permissions::Owner::NONE);
    }
  }
}
