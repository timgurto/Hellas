#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Simple quest", "[quests]") {
  GIVEN("a quest starting at A and ending at B") {
    auto data = R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="questFromAToB" startsAt="A" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});
    const auto &a = s.getFirstObject();

    s.addObject("B", {15, 10});
    auto b = a.serial() + 1;

    WHEN("a  client accepts the quest from A") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", a.serial()));

      THEN("he is on a quest") { WAIT_UNTIL(user.numQuests() == 1); }

      AND_WHEN("he completes the quest at B") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("questFromAToB", b));

        THEN("he is not on a quest") { WAIT_UNTIL(!user.numQuests() == 0); }
      }
    }
  }
}

TEST_CASE("Cases where a quest should not be accepted", "[quests]") {
  auto data = R"(
    <objectType id="A" />
    <objectType id="B" />
    <objectType id="D" />
    <quest id="questFromAToB" startsAt="A" endsAt="B" />
  )";
  auto s = TestServer::WithDataString(data);
  auto c = TestClient::WithDataString(data);
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

TEST_CASE("Cases where a quest should not be completed", "[quests]") {
  auto data = R"(
    <objectType id="A" />
    <objectType id="B" />
    <quest id="questFromAToB" startsAt="A" endsAt="B" />
  )";
  auto s = TestServer::WithDataString(data);
  auto c = TestClient::WithDataString(data);

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
    auto invalidSerial = 50;
    c.sendMessage(CL_COMPLETE_QUEST, makeArgs(invalidSerial));
  }

  SECTION("The object is the wrong type") {
    c.sendMessage(CL_COMPLETE_QUEST, makeArgs("questFromAToB", a.serial()));
  }

  // Then he is still on the quest
  REPEAT_FOR_MS(100);
  CHECK(user.numQuests() == 1);
}

TEST_CASE("Identical source and destination", "[quests]") {
  // Given two quests that start at A and end at B
  auto data = R"(
    <objectType id="A" />
    <objectType id="B" />
    <quest id="quest1" startsAt="A" endsAt="B" />
    <quest id="quest2" startsAt="A" endsAt="B" />
  )";
  auto s = TestServer::WithDataString(data);
  auto c = TestClient::WithDataString(data);

  // And an object, A
  s.addObject("A", {10, 15});
  const auto &a = s.getFirstObject();

  // And an object, B
  s.addObject("B", {15, 10});
  auto b = a.serial() + 1;

  // When a client accepts both quests
  s.waitForUsers(1);
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", a.serial()));
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest2", a.serial()));

  // Then he is on two quest
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.numQuests() == 2);

  // And when he completes the quests at B
  c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", b));
  c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest2", b));

  // Then he is not on any quests
  REPEAT_FOR_MS(100);
  CHECK(user.numQuests() == 0);
}

TEST_CASE("Client knows about objects' quests", "[quests]") {
  auto data = R"(
    <objectType id="A" />
    <quest id="quest1" startsAt="A" endsAt="A" />
    <quest id="quest2" startsAt="A" endsAt="A" />
  )";
  auto s = TestServer::WithDataString(data);

  // Given an object, A
  s.addObject("A", {10, 15});

  // When a client logs in
  auto c = TestClient::WithDataString(data);
  s.waitForUsers(1);

  // Then he knows that the object has two quests
  WAIT_UNTIL(c.objects().size() == 1);
  const auto &a = c.getFirstObject();
  WAIT_UNTIL(a.startsQuests().size() == 2);

  // And he knows that it gives both quests
  auto hasQuest1 = false, hasQuest2 = false;
  for (auto quest : a.startsQuests()) {
    if (quest->id() == "quest1")
      hasQuest1 = true;
    else if (quest->id() == "quest2")
      hasQuest2 = true;
  }
  CHECK(hasQuest1);
  CHECK(hasQuest2);
}

TEST_CASE("Client knows when quests can be completed", "[quests]") {
  GIVEN("an object that starts and ends a quest") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    WHEN("a user connects") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 1);
      const auto &obj = c.getFirstObject();

      THEN("he knows he can't complete any quests at the object") {
        WAIT_UNTIL(obj.endsQuests().size() == 0);
      }

      AND_WHEN("he accepts the quest") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", serial));

        THEN("he knows that he can complete it at the object") {
          WAIT_UNTIL(obj.endsQuests().size() == 1);
        }
      }
    }
  }
}

TEST_CASE("Client knows when objects have no quests", "[quests]") {
  // Given an object type B, with no quests
  auto data = R"(
    <objectType id="B" />
  )";
  auto s = TestServer::WithDataString(data);

  // And an object, B
  s.addObject("B", {10, 15});

  // When a client logs in
  auto c = TestClient::WithDataString(data);
  s.waitForUsers(1);

  // Then he knows that the object has no quests
  WAIT_UNTIL(c.objects().size() == 1);
  const auto &b = c.getFirstObject();
  REPEAT_FOR_MS(100);
  CHECK(b.startsQuests().empty());
}

TEST_CASE("A user can't pick up a quest he's already on", "[quests]") {
  GIVEN("a user is already on the quest that A offers") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest("quest1");

    WHEN("an A object is added") {
      s.addObject("A", {10, 15});
      WAIT_UNTIL(c.objects().size() == 1);

      THEN("he is told that there are no quests available") {
        const auto &a = c.getFirstObject();
        CHECK(a.startsQuests().size() == 0);
      }
    }
  }
}

TEST_CASE("After a user accepts a quest, he can't do so again", "[quests]") {
  auto data = R"(
    <objectType id="A" />
    <quest id="quest1" startsAt="A" endsAt="A" />
    <quest id="quest2" startsAt="A" endsAt="A" />
  )";
  auto s = TestServer::WithDataString(data);
  auto c = TestClient::WithDataString(data);

  // Given an object, A
  s.addObject("A", {10, 15});
  auto serial = s.getFirstObject().serial();

  // When a client accepts a quest from A
  s.waitForUsers(1);
  c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", serial));
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.numQuests() == 1);

  // Then he can see only one quest at object A
  REPEAT_FOR_MS(100);
  const auto &a = c.getFirstObject();
  CHECK(a.startsQuests().size() == 1);
}

TEST_CASE("Quest UI", "[quests][ui]") {
  GIVEN("an object that gives a quest") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});

    WHEN("a client logs in") {
      auto c = TestClient::WithDataString(data);

      const auto &quest = c.getFirstQuest();

      WAIT_UNTIL(c.objects().size() == 1);
      const auto &obj = c.getFirstObject();

      THEN("the quest has no window") { CHECK(quest.window() == nullptr); }

      AND_THEN("the object has an object window") {
        WAIT_UNTIL(obj.window() != nullptr);
      }

      AND_WHEN("the quest button is clicked") {
        REPEAT_FOR_MS(100);
        auto questButtonE = obj.window()->findChild("quest1");
        auto questButton = dynamic_cast<Button *>(questButtonE);
        CHECK(questButton != nullptr);

        questButton->depress();
        questButton->release(true);

        THEN("the quest has a visible window") {
          WAIT_UNTIL(quest.window() != nullptr);
          CHECK(quest.window()->visible());
          CHECK(c->isWindowRegistered(quest.window()));
        }

        AND_WHEN("the \"Accept\" button is clicked") {
          auto acceptButtonE = quest.window()->findChild("accept");
          auto acceptButton = dynamic_cast<Button *>(acceptButtonE);
          CHECK(acceptButton != nullptr);

          acceptButton->depress();
          acceptButton->release(true);

          THEN("the user is on a quest") {
            auto &user = s.getFirstUser();
            WAIT_UNTIL(user.numQuests() == 1);
          }
          /*
          AND_THEN(
              "the object window is still visible (because the quest can be "
              "completed)") {
            REPEAT_FOR_MS(100);
            CHECK(obj.window()->visible());
          }
          */
        }
      }
    }
  }
}

TEST_CASE("Quest UI for NPCs", "[quests][ui]") {
  GIVEN("an NPC that gives a quest") {
    auto data = R"(
      <npcType id="A" maxHealth="1" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addNPC("A", {10, 15});

    WHEN("a client logs in") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 1);
      auto &npc = c.getFirstNPC();

      THEN("The NPC has an object window") {
        WAIT_UNTIL(npc.window() != nullptr);
      }

      AND_WHEN("The client right-clicks on the NPC") {
        npc.onRightClick(c.client());

        THEN("The object window is visible") {
          WAIT_UNTIL(npc.window()->visible());
        }
      }
    }
  }
}

TEST_CASE("Show the user when an object has no more quests", "[quests]") {
  GIVEN("an object that gives a quest") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    WHEN("a client accepts the quest") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 1);
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", serial));

      THEN("The client sees that the object has no quests left") {
        const auto &obj = c.getFirstObject();
        WAIT_UNTIL(obj.startsQuests().size() == 0);
      }
    }
  }
}
