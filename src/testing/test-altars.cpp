#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("A user can't build multiple player-unique objects", "[player-unique]") {
    // Given "blonde" and "readhead" object types,
    // And each is marked with the "wife" player-unique category,
    auto s = TestServer::WithData("wives");

    // And Bob has a blonde wife
    s.addObject("blonde", {}, "bob");

    SECTION("Bob tries for a second wife") {
        // When Bob logs in,
        auto c = TestClient::WithUsernameAndData("bob", "wives");
        WAIT_UNTIL(s.users().size() == 1);

        // And tries to get a readhead wife
        c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 15));

        // Then there is still only one in the world
        REPEAT_FOR_MS(100);
        CHECK(s.entities().size() == 1);
    }

    SECTION("Charlie wants a wife too") {
        // When Charlie logs in,
        auto c = TestClient::WithUsernameAndData("charlie", "wives");
        WAIT_UNTIL(s.users().size() == 1);
        auto &user = s.getFirstUser();

        // And tries to get a readhead wife
        c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 15, 15));

        // Then there are now two (one each)
        REPEAT_FOR_MS(100);
        CHECK(s.entities().size() == 2);
    }
}
