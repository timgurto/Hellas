#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("No erroneous transform messages on login")
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(20,20));
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);

    bool transformTimeReceived = c.waitForMessage(SV_TRANSFORM_TIME, 200);
    return ! transformTimeReceived;
TEND
