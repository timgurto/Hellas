#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("A user can't build multiple player-unique objects") {
  // Given "blonde" and "readhead" object types,
  // And each is marked with the "wife" player-unique category,
  auto s = TestServer::WithData("wives");

  // And Bob has a blonde wife
  s.addObject("blonde", {}, "Bob");

  SECTION("Bob can't have a second wife") {
    // When Bob logs in,
    auto c = TestClient::WithUsernameAndData("Bob", "wives");
    s.waitForUsers(1);

    // And tries to get a readhead wife
    c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 15));

    // Then Bob receives an error message,
    c.waitForMessage(WARNING_UNIQUE_OBJECT);

    // And there is still only one in the world
    CHECK(s.entities().size() == 1);
  }

  SECTION("Charlie can have a wife too") {
    // When Charlie logs in,
    auto c = TestClient::WithUsernameAndData("Charlie", "wives");
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    // And tries to get a readhead wife
    c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 15, 15));
    REPEAT_FOR_MS(100);

    // Then there are now two (one each)
    CHECK(s.entities().size() == 2);
  }

  SECTION("Bob can't give his wife to the city", "[city]") {
    // And there is a city of Athens
    s.cities().createCity("Athens");

    // When Bob logs in,
    auto c = TestClient::WithUsernameAndData("Bob", "wives");
    s.waitForUsers(1);

    // And joins Athens,
    auto &bob = s.getFirstUser();
    s.cities().addPlayerToCity(bob, "Athens");

    // And tries to give his wife to Athens
    auto &wife = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(wife.serial()));

    // Then Bob receives an error message,
    c.waitForMessage(ERROR_CANNOT_CEDE);

    // And the wife still belongs to him
    CHECK(wife.permissions().isOwnedByPlayer("Bob"));
  }

  SECTION("If Bob's wife dies he can get a new one") {
    // Given Bob's wife is dead
    auto &firstWife = s.getFirstObject();
    firstWife.reduceHealth(firstWife.health());

    // When Bob logs in,
    auto c = TestClient::WithUsernameAndData("Bob", "wives");
    s.waitForUsers(1);

    // And tries to get a readhead wife
    c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 15));

    // Then there are two wives
    WAIT_UNTIL(s.entities().size() == 2);
  }
}

TEST_CASE("Clients can discern player-uniqueness") {
  auto c = TestClient::WithData("wives");
  const auto &blonde = c.getFirstObjectType();
  CHECK(blonde.isPlayerUnique());
  CHECK(blonde.hasTags());
}

TEST_CASE("End-of-tutorial altar") {
  GIVEN("an altar that ends the tutorial, and a user next to it") {
    auto data = R"(
      <objectType id="altar">
        <action target="endTutorial" />
      </objectType>
      <postTutorialSpawn x="20" y="20" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("altar", {10, 15});
    const auto &altar = s.getFirstObject();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    auto oldLocation = user.location();
    const auto expectedLocation = MapPoint{20, 20};

    THEN("an altar can be added") { s.addObject("altar", {10, 15}); }

    WHEN("a user worships there") {
      c.sendMessage(CL_PERFORM_OBJECT_ACTION, makeArgs(altar.serial(), "_"s));

      THEN("he is at the new specified location") {
        WAIT_UNTIL(user.location() != oldLocation);
        CHECK(user.location() == expectedLocation);
      }
    }
  }
}
