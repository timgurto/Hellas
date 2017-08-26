#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("A user can't build multiple player-unique objects", "[player-unique]") {
    // Given "blonde" and "readhead" object types,
    // And each is marked with the "wife" player-unique category,
    auto s = TestServer::WithData("wives");

    // And Bob has a blonde wife
    s.addObject("blonde", {}, "bob");

    SECTION("Bob can't have a second wife") {
        // When Bob logs in,
        auto c = TestClient::WithUsernameAndData("bob", "wives");
        WAIT_UNTIL(s.users().size() == 1);

        // And tries to get a readhead wife
        c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 15));

        // Then Bob receives an error message,
        c.waitForMessage(SV_PLAYER_UNIQUE_OBJECT);

        // And there is still only one in the world
        CHECK(s.entities().size() == 1);
    }

    SECTION("Charlie can have a wife too") {
        // When Charlie logs in,
        auto c = TestClient::WithUsernameAndData("charlie", "wives");
        WAIT_UNTIL(s.users().size() == 1);
        auto &user = s.getFirstUser();

        // And tries to get a readhead wife
        c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 15, 15));
        REPEAT_FOR_MS(100);

        // Then there are now two (one each)
        CHECK(s.entities().size() == 2);
    }

    SECTION("Bob can't give his wife to the city", "[city]") {
        // And there is a city of Athens
        s.cities().createCity("athens");

        // When Bob logs in,
        auto c = TestClient::WithUsernameAndData("bob", "wives");
        WAIT_UNTIL(s.users().size() == 1);

        // And joins Athens,
        auto &bob = s.getFirstUser();
        s.cities().addPlayerToCity(bob, "athens");

        // And tries to give his wife to Athens
        auto &wife = s.getFirstObject();
        c.sendMessage(CL_CEDE, makeArgs(wife.serial()));

        // Then Bob receives an error message,
        c.waitForMessage(SV_CANNOT_CEDE);

        // And the wife still belongs to him
        CHECK(wife.permissions().isOwnedByPlayer("bob"));
    }
}

TEST_CASE("Clients can discern player uniqueness", "[player-unique]") {
    auto c = TestClient::WithData("wives");
    c.getFirstObjectType().isPlayerUnique();
}
