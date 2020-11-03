#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Start and stop server") { TestServer server; }

TEST_CASE("Concurrent clients") {
  GIVEN("two clients") {
    auto s = TestServer{};
    auto c1 = TestClient{};
    auto c2 = TestClient{};

    THEN("both successfully log into the server") { s.waitForUsers(2); }
  }
}

TEST_CASE("Run TestClient with custom username") {
  // Given a server
  TestServer s;

  // When a TestClient is created with a custom username
  TestClient alice = TestClient::WithUsername("Alice");
  s.waitForUsers(1);

  // Then the client logs in with that username
  CHECK(alice->username() == "Alice");
}

TEST_CASE("Removed users are removed from co-ord indices") {
  // Given a server
  TestServer s;

  // When a client connects and disconnects
  { TestClient c; }
  s.waitForUsers(0);

  // Then that user is not represented in the x-indexed objects list
  CHECK(s.entitiesByX().empty());
}

TEST_CASE("Server remains functional with unresponsive client",
          "[.slow][.flaky]") {
  // Given a server and client
  TestServer s;
  {
    TestClient badClient;
    s.waitForUsers(1);

    // When the client freezes and becomes unresponsive;
    badClient.freeze();

    // And times out on the server
    WAIT_UNTIL_TIMEOUT(s.users().size() == 0, 15000);
  }

  // Then the server can still accept connections
  TestClient goodClient;
  s.waitForUsers(1);
}

TEST_CASE("Map with extra row doesn't crash client", "[.flaky]") {
  TestServer s;
  TestClient c = TestClient::WithData("abort");
  REPEAT_FOR_MS(1000);
}

TEST_CASE("New servers clear old user data") {
  {
    TestServer s;
    TestClient c = TestClient::WithUsername("Alice");
    s.waitForUsers(1);
    User &alice = s.getFirstUser();
    CHECK(alice.health() == alice.stats().maxHealth);
    alice.reduceHealth(1);
    CHECK(alice.health() < alice.stats().maxHealth);
  }
  {
    TestServer s;
    TestClient c = TestClient::WithUsername("Alice");
    s.waitForUsers(1);
    User &alice = s.getFirstUser();
    CHECK(alice.health() == alice.stats().maxHealth);
  }
}

TEST_CASE("The client handles a server appearing") {
  // Given a client
  auto c = TestClient{};

  REPEAT_FOR_MS(4000);

  // When a server appears
  auto s = TestServer{};

  // Then the client connects to it
  s.waitForUsers(1);
}

TEST_CASE("Online-players list includes existing players") {
  GIVEN("a server and client") {
    auto s = TestServer{};
    auto c1 = TestClient{};
    s.waitForUsers(1);

    WHEN("another client connects") {
      auto c2 = TestClient{};

      THEN("it knows there are two players online") {
        WAIT_UNTIL(c2.allOnlinePlayers().size() == 2);
      }
    }
  }
}

TEST_CASE("Windows close on disconnect") {
  // Given a client with windows open
  auto c = TestClient{};
  {
    auto s = TestServer{};
    WAIT_UNTIL(c.connected());
    c.buildWindow()->show();
    c.craftingWindow()->show();
    CHECK(c.buildWindow()->visible());

    // When the server goes offline
  }

  // And when the client disconnects
  WAIT_UNTIL_TIMEOUT(!c.connected(), 15000);

  // Then the client's windows are closed
  WAIT_UNTIL(!c.buildWindow()->visible());
  WAIT_UNTIL(!c.craftingWindow()->visible());
}

TEST_CASE("Accurate health/energy on login") {
  auto s = TestServer{};

  // GIVEN Alice has half health
  {
    auto c = TestClient::WithUsername("Alice");
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.reduceHealth(10);
    user.reduceEnergy(10);

    // WHEN she logs in
  }
  {
    auto c = TestClient::WithUsername("Alice");

    // THEN she knows she isn't on full health
    WAIT_UNTIL(c->character().health() < c->character().maxHealth());
    WAIT_UNTIL(c->character().energy() < c->character().maxEnergy());
  }
}

TEST_CASE("The finished-loggin-in message") {
  GIVEN("a client connects to a server") {
    auto s = TestServer{};
    auto c = TestClient{};
    CHECK(c.waitForMessage(SV_LOGIN_INFO_HAS_FINISHED));
  }
}
