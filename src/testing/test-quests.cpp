#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

auto simpleQuests = R"(
  <objectType id="A" />
  <objectType id="B" />
  <objectType id="D" />

  <quest id="questFromAToB" startsAt="A" endsAt="B" />
  <quest id="quest2FromAToB" startsAt="A" endsAt="B" />
  <quest id="questFromD" startsAt="D" endsAt="D" />
)";

TEST_CASE("Simple quest") {
  auto s = TestServer::WithDataString(simpleQuests);
  auto c = TestClient::WithDataString(simpleQuests);

  // Given an object, A
  s.addObject("A", {10, 15});
  const auto &a = s.getFirstObject();

  // And an object, B
  s.addObject("B", {15, 10});
  auto b = a.serial() + 1;

  // When a client accepts a quest from A
  s.waitForUsers(1);
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", a.serial()));

  // Then he is on a quest
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.numQuests() == 1);

  // When he completes the quest at B
  c.sendMessage(CL_COMPLETE_QUEST, makeArgs("questFromAToB", b));

  // Then he is not on a quest
  WAIT_UNTIL(!user.numQuests() == 0);
}

TEST_CASE("Cases where a quest should not be accepted") {
  auto s = TestServer::WithDataString(simpleQuests);
  auto c = TestClient::WithDataString(simpleQuests);
  s.waitForUsers(1);

  SECTION("No attempt is made to accept a quest") {}

  SECTION("The client tries to accept a quest from a nonexistent object") {
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", 50));
  }

  SECTION("The object is too far away") {
    s.addObject("A", {100, 100});
    const auto &a = s.getFirstObject();
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", a.serial()));
  }

  SECTION("The object does not have a quest") {
    s.addObject("B", {10, 15});
    const auto &b = s.getFirstObject();
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", b.serial()));
  }

  SECTION("The object has the wrong quest") {
    s.addObject("D", {10, 15});
    const auto &d = s.getFirstObject();
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", d.serial()));
  }

  // Then user is not on a quest
  auto &user = s.getFirstUser();
  REPEAT_FOR_MS(100);
  CHECK(user.numQuests() == 0);
}

TEST_CASE("Cases where a quest should not be completed") {
  auto s = TestServer::WithDataString(simpleQuests);
  auto c = TestClient::WithDataString(simpleQuests);

  // Given an object, A
  s.addObject("A", {10, 15});
  const auto &a = s.getFirstObject();

  // And the user has accepted a quest from A
  s.waitForUsers(1);
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", a.serial()));

  // And he is therefore on a quest
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.numQuests() == 1);

  SECTION("The client tries to complete a quest at a nonexistent object") {
    c.sendMessage(CL_COMPLETE_QUEST, makeArgs(50));
  }

  SECTION("The object is the wrong type") {
    c.sendMessage(CL_COMPLETE_QUEST, makeArgs("questFromAToB", a.serial()));
  }

  // Then he is still on the quest
  REPEAT_FOR_MS(100);
  CHECK(user.numQuests() == 1);
}

TEST_CASE("Identical source and destination") {
  // Given two quests that start at A and end at B
  auto s = TestServer::WithDataString(simpleQuests);
  auto c = TestClient::WithDataString(simpleQuests);

  // And an object, A
  s.addObject("A", {10, 15});
  const auto &a = s.getFirstObject();

  // And an object, B
  s.addObject("B", {15, 10});
  auto b = a.serial() + 1;

  // When a client accepts two quests from A (that finish at B)
  s.waitForUsers(1);
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", a.serial()));
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest2FromAToB", a.serial()));

  // Then he is on two quest
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.numQuests() == 2);

  // When he completes the quests at B
  c.sendMessage(CL_COMPLETE_QUEST, makeArgs("questFromAToB", b));
  c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest2FromAToB", b));

  // Then he is not on any quests
  REPEAT_FOR_MS(100);
  CHECK(user.numQuests() == 0);
}

TEST_CASE("Client knows about objects' quests") {
  auto s = TestServer::WithDataString(simpleQuests);

  // Given an object, A
  s.addObject("A", {10, 15});

  // When a client logs in
  auto c = TestClient::WithDataString(simpleQuests);
  s.waitForUsers(1);

  // Then he knows that the object has two quests
  WAIT_UNTIL(c.objects().size() == 1);
  const auto &a = c.getFirstObject();
  WAIT_UNTIL(a.startsQuests().size() == 2);

  // And he knows that it gives "quest2FromAToB"
  CHECK(a.startsQuests().find("quest2FromAToB") != a.startsQuests().end());
}

TEST_CASE("Client knows when objects have no quests") {
  auto s = TestServer::WithDataString(simpleQuests);

  // Given an object, B
  s.addObject("B", {10, 15});

  // When a client logs in
  auto c = TestClient::WithDataString(simpleQuests);
  s.waitForUsers(1);

  // Then he knows that the object has no quests
  WAIT_UNTIL(c.objects().size() == 1);
  const auto &b = c.getFirstObject();
  REPEAT_FOR_MS(100);
  CHECK(b.startsQuests().empty());
}

TEST_CASE("A user can't pick up a quest he's already on") {
  auto s = TestServer::WithDataString(simpleQuests);
  auto c = TestClient::WithDataString(simpleQuests);

  // Given the user has accepted a quest from A
  s.waitForUsers(1);
  auto &user = s.getFirstUser();
  user.startQuest("questFromAToB");

  // When an object, A, is added (which has two quests)
  s.addObject("A", {10, 15});

  // Then the user is told that there is only one quest available
  REPEAT_FOR_MS(100);
  const auto &a = c.getFirstObject();
  CHECK(a.startsQuests().size() == 1);
}

TEST_CASE("After a user accepts a quest, he can't do so again") {
  auto s = TestServer::WithDataString(simpleQuests);
  auto c = TestClient::WithDataString(simpleQuests);

  // Given an object, A
  s.addObject("A", {10, 15});
  auto serial = s.getFirstObject().serial();

  // When a client accepts a quest from A
  s.waitForUsers(1);
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", serial));
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.numQuests() == 1);

  // Then he can see only one quest at object A
  REPEAT_FOR_MS(100);
  const auto &a = c.getFirstObject();
  CHECK(a.startsQuests().size() == 1);
}
