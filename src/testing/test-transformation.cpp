#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("No erroneous transform messages on login", "") {
  TestServer s = TestServer::WithData("basic_rock");
  s.addObject("rock", {20, 20});
  TestClient c = TestClient::WithData("basic_rock");
  s.waitForUsers(1);

  bool transformTimeReceived = c.waitForMessage(SV_TRANSFORM_TIME, 200);
  CHECK_FALSE(transformTimeReceived);
}
