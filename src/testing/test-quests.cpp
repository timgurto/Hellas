#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Simple quest") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    // When a client accepts a quest from A
    c.sendMessage(CL_ACCEPT_QUEST);

    auto &user = s.getFirstUser();
    WAIT_UNTIL(user.isOnQuest());
}

TEST_CASE("Not on a quest") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    auto &user = s.getFirstUser();
    REPEAT_FOR_MS(100)
        ;
    CHECK(!user.isOnQuest());
}
