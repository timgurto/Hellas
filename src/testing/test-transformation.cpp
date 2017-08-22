#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("No erroneous transform messages on login", "[.flaky-vs2015]"){
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(20,20));
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);

    bool transformTimeReceived = c.waitForMessage(SV_TRANSFORM_TIME, 200);
    CHECK_FALSE(transformTimeReceived);
}
