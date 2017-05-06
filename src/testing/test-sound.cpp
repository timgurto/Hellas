#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("A missing sound variabt fails gracefully")
    TestServer s;
    TestClient c = TestClient::WithData("missing_sound");
    WAIT_UNTIL(s.users().size() == 1);

    c->character().playAttackSound();

    return true;
TEND
