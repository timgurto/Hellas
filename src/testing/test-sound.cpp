#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("A missing sound variant fails gracefully"){
    // Given a server, and a client with a missing sound variant
    TestServer s;
    TestClient c = TestClient::WithData("missing_sound");
    WAIT_UNTIL(s.users().size() == 1);

    // When the client attempts to play that sound
    // Then the test doesn't crash.
    c->character().playAttackSound();
}
