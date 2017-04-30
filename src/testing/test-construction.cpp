#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"

TEST("Construction materials can be added")
    TestServer s = TestServer::Data("brick_wall");
    TestClient c = TestClient::Data("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.objects().size() == 1);

    return true;
TEND
