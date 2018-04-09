#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Simple quest") {
    auto s = TestServer::WithData("simpleQuest");
    auto c = TestClient::WithData("simpleQuest");

    // Given an object, A
    s.addObject("A", { 10, 15 });
    const auto &a = s.getFirstObject();

    // When a client accepts a quest from A
    s.waitForUsers(1);
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs(a.serial()));

    // Then he is on a quest
    auto &user = s.getFirstUser();
    WAIT_UNTIL(user.isOnQuest());
}

TEST_CASE("Invalid quest") {
    auto s = TestServer::WithData("simpleQuest");
    auto c = TestClient::WithData("simpleQuest");
    s.waitForUsers(1);

    SECTION("When the client is not on a quest") {}

    SECTION("When the client tries to accept a quest from a nonexistent object") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs(50));
    }

    SECTION("When the object is too far away") {
        s.addObject("A", { 100, 100 });
        const auto &a = s.getFirstObject();
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs(a.serial()));
    }

    // Then user is not on a quest
    auto &user = s.getFirstUser();
    REPEAT_FOR_MS(100)
        ;
    CHECK(!user.isOnQuest());
}
