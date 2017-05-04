#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("No erroneous transform messages on login")
    TestServer s = TestServer::Data("basic_rock");
    s.addObject("rock", Point(20,20));
    TestClient c = TestClient::Data("basic_rock");

    bool transformTimeReceived = c.waitForMessage(SV_TRANSFORM_TIME, 200);
    return ! transformTimeReceived;
TEND
