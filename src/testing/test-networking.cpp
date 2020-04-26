#include <cstdio>

#include "../Socket.h"
#include "../curlUtil.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Read invalid URL", "[.slow]") {
  CHECK(readFromURL("fake.fake.fake").empty());
}

TEST_CASE("Read badly-formed URL", "[.slow]") {
  CHECK(readFromURL("1").empty());
}

TEST_CASE("Read blank URL") { CHECK(readFromURL("").empty()); }

TEST_CASE("Read test URL", "[.slow]") {
  CHECK(readFromURL("timgurto.com/test.txt") ==
        "This is a file for testing web-access accuracy.");
}

TEST_CASE("Use socket after cleanup") {
  {
    TestServer s;
    TestClient c;

    WAIT_UNTIL(c.connected());

    // Winsock should be cleaned up here.
  }
  {
    TestServer s;
    TestClient c;

    WAIT_UNTIL(c.connected());
  }
}

TEST_CASE("Download file") {
  downloadFile("timgurto.com/test.txt", "test.txt");

  {
    auto f = std::ifstream("test.txt");
    REQUIRE(f.good());

    auto word = std::string{};
    f >> word;
    CHECK(word == "This");
  }

  remove("test.txt");
}

TEST_CASE("Bulk messages") {
  GIVEN("a large number of recipes") {
    AND_GIVEN("a user unlocks them all") {
      THEN("the client knows the correct number of recipes") {}
    }
  }
}
