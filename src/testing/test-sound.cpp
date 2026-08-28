#include "TestClient.h"
#include "testing.h"
#include "TestServer.h"

TEST_CASE("A missing sound variant fails gracefully") {
  // Given a server, and a client with a missing sound variant
  TestServer s;
  TestClient c = TestClient::WithDataString(R"(
    <soundProfile id="avatar" >
      <sound type="attack" file="doesntExist" />
    </soundProfile>
  )");
  s.waitForUsers(1);

  // When the client attempts to play that sound
  // Then the test doesn't crash.
  c->character().playAttackSound();
}
