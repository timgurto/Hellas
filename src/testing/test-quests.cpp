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

    // When he completes the quest
    c.sendMessage(CL_COMPLETE_QUEST, makeArgs(a.serial()));

    // Then he is not on a quest
    REPEAT_FOR_MS(100)
        ;
    CHECK(!user.isOnQuest());
}

TEST_CASE("Cases where a quest should not be accepted") {
    auto s = TestServer::WithData("simpleQuest");
    auto c = TestClient::WithData("simpleQuest");
    s.waitForUsers(1);

    SECTION("No attempt is made to accept a quest") {}

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

TEST_CASE("Cases where a quest should not be completed") {
    auto s = TestServer::WithData("simpleQuest");
    auto c = TestClient::WithData("simpleQuest");

    // Given an object, A
    s.addObject("A", { 10, 15 });
    const auto &a = s.getFirstObject();

    // And the user has accepted a quest from A
    s.waitForUsers(1);
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromA", a.serial()));

    // And he is therefore on a quest
    auto &user = s.getFirstUser();
    WAIT_UNTIL(user.isOnQuest());

    SECTION("The client tries to complete a quest at a nonexistent object") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs(50));
    }

    // Then he is still on the quest
    REPEAT_FOR_MS(100)
        ;
    CHECK(user.isOnQuest());
}
