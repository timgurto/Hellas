#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("A user can't build multiple player-unique objects", "[player-unique]") {
    // Given "blonde" and "readhead" object types, each buildable with an engagement ring;
    // And each is marked with the "wife" player-unique category;
    auto s = TestServer::WithData("wives");

    // And a user named Bob
    auto c = TestClient::WithUsernameAndData("bob", "wives");

    // And Bob has a blonde wife
    s.addObject("blonde", {}, "bob");

    // And Bob has an engagement ring;
    WAIT_UNTIL(s.users().size() == 1);
    auto &user = s.getFirstUser();
    auto &engagementRing = s.getFirstItem();
    user.giveItem(&engagementRing);

    // When Bob tries to get a readhead wife
    c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 10));

    // Then there is still only one in the world
    REPEAT_FOR_MS(100);
    CHECK(s.entities().size() == 1);
}
