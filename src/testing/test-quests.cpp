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
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromA", a.serial()));

    // Then he is on a quest
    auto &user = s.getFirstUser();
    WAIT_UNTIL(user.isOnQuest());
}

TEST_CASE("Invalid quest") {
    auto s = TestServer::WithData("simpleQuest");
    auto c = TestClient::WithData("simpleQuest");
    s.waitForUsers(1);

    SECTION("The client is not on a quest") {}

    SECTION("The client tries to accept a quest from a nonexistent object") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromA", 50));
    }

    SECTION("The object is too far away") {
        s.addObject("A", { 100, 100 });
        const auto &a = s.getFirstObject();
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromA", a.serial()));
    }

    SECTION("The object does not have a quest") {
        s.addObject("B", { 10, 15 });
        const auto &b = s.getFirstObject();
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromA", b.serial()));
    }

    SECTION("The object has the wrong quest") {
        s.addObject("D", { 10, 15 });
        const auto &d = s.getFirstObject();
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromA", d.serial()));
    }

    // Then user is not on a quest
    auto &user = s.getFirstUser();
    REPEAT_FOR_MS(100)
        ;
    CHECK(!user.isOnQuest());
}
