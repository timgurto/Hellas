#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Clients can be created with a certain class") {
  auto s = TestServer::WithDataString("");
  auto c = TestClient::WithClassAndDataString("Test", "");
}
