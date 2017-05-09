#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("A missing sound variant fails gracefully")
    // Given a server, and a client with a missing sound variant
    TestServer s;
    TestClient c = TestClient::WithData("missing_sound");
    WAIT_UNTIL(s.users().size() == 1);

    // When the client attempts to play that sound
    c->character().playAttackSound();

    // Then the test doesn't crash.
    return true;
TEND
