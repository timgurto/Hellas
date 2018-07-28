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

TEST_CASE("A user must be on a quest to complete it", "[quests]") {
  GIVEN("A user, quest and quest node") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    const auto &user = s.getFirstUser();

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    WHEN("he tries to complete the quest at the node") {
      c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

      THEN("he has not completed the quest") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(user.hasCompletedQuest("quest1"));
      }
    }
  }
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
    if (quest->info().id == "quest1")
      hasQuest1 = true;
    else if (quest->info().id == "quest2")
      hasQuest2 = true;
  }
  CHECK(hasQuest1);
  CHECK(hasQuest2);
}

TEST_CASE("Clients know quests' correct end nodes", "[quests]") {
  GIVEN("a quest from A to B") {
    auto data = R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="quest1" startsAt="A" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});
    const auto &a = s.getFirstObject();
    auto aSerial = a.serial();

    s.addObject("B", {15, 10});
    auto bSerial = aSerial + 1;

    WHEN("a user accepts the quest") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 2);
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", aSerial));

      THEN("he knows object B ends it") {
        auto &b = c.objects()[bSerial];
        REQUIRE(b != nullptr);
        WAIT_UNTIL(b->completableQuests().size() == 1);
      }
    }
  }
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
        WAIT_UNTIL(obj.completableQuests().size() == 0);
      }

      AND_WHEN("he accepts the quest") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", serial));

        THEN("he knows that he can complete it at the object") {
          WAIT_UNTIL(obj.completableQuests().size() == 1);
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
    user.startQuest(s.getFirstQuest());

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
      auto &obj = c.getFirstObject();

      THEN("the quest has a window") { CHECK(quest.window() == nullptr); }

      AND_WHEN("the quest button is clicked") {
        obj.onRightClick(c.client());
        WAIT_UNTIL(obj.window() != nullptr);

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
              "the object window is still visible (because the quest can be
          " "completed)") { REPEAT_FOR_MS(100);
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

      AND_WHEN("The client right-clicks on the NPC") {
        npc.onRightClick(c.client());

        THEN("The object window is visible") {
          WAIT_UNTIL(npc.window() != nullptr);
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

TEST_CASE("A quest to kill an NPC", "[quests]") {
  GIVEN("A quest that requires a rat to be killed, and a rat") {
    auto data = R"(
      <objectType id="A" />
      <npcType id="rat" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective id="rat" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);
    auto &u = s.getFirstUser();

    s.addObject("A", {10, 5});
    auto aSerial = s.getFirstObject().serial();

    s.addNPC("rat", {10, 15});
    auto ratSerial = aSerial + 1;
    auto rat = s.entities().find(ratSerial);
    WAIT_UNTIL(c.objects().size() == 2);
    const auto a = c.objects()[aSerial];

    WHEN("the user is on the quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", aSerial));
      WAIT_UNTIL(u.numQuests() == 1);

      THEN("the user can see no completable quests at A") {
        REPEAT_FOR_MS(100);
        CHECK(a->completableQuests().size() == 0);
      }

      AND_WHEN("the user tries to complete the quest") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));
        THEN("he is still on the quest") {
          REPEAT_FOR_MS(100);
          CHECK(u.numQuests() == 1);
        }
      }

      AND_WHEN("he kills a rat") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(ratSerial));
        WAIT_UNTIL(rat->isDead());

        THEN("he can see a completable quest at A") {
          WAIT_UNTIL(a->completableQuests().size() == 1);
        }

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));

          THEN("he is not on the quest") { WAIT_UNTIL(u.numQuests() == 0); }
        }
      }

      AND_WHEN("there is also a \"mouse\" NPC") {
        auto extraData = R"(
          <npcType id="mouse" />
        )";
        s.loadDataFromString(extraData);
        c.loadDataFromString(extraData);

        s.addNPC("mouse", {5, 10});
        auto mouseSerial = ratSerial + 1;
        auto mouse = s.entities().find(mouseSerial);

        AND_WHEN("the user kills the mouse") {
          c.sendMessage(CL_TARGET_ENTITY, makeArgs(mouseSerial));
          WAIT_UNTIL(mouse->isDead());

          AND_WHEN("he tries to complete the quest") {
            c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));

            THEN("he is still on the quest") {
              REPEAT_FOR_MS(100);
              CHECK(u.numQuests() == 1);
            }
          }
        }
      }
    }

    WHEN("the user kills a rat while not on the quest") {
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(ratSerial));
      WAIT_UNTIL(rat->isDead());
      REPEAT_FOR_MS(100);

      AND_WHEN("he starts the quest") {
        u.startQuest(s.getFirstQuest());

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));

          THEN("he is still on the quest") {
            REPEAT_FOR_MS(100);
            CHECK(u.numQuests() == 1);
          }
        }
      }
    }
  }

  GIVEN("A quest that requires a mouse to be killed, and a mouse") {
    auto data = R"(
      <objectType id="A" />
      <npcType id="mouse" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective id="mouse" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);
    auto &u = s.getFirstUser();

    s.addObject("A", {10, 5});
    auto aSerial = s.getFirstObject().serial();
    WAIT_UNTIL(c.objects().size() == 1);
    auto &a = c.getFirstObject();

    s.addNPC("mouse", {10, 15});
    auto mouseSerial = aSerial + 1;
    auto mouse = s.entities().find(mouseSerial);
    WAIT_UNTIL(c.objects().size() == 2);

    WHEN("the user is on the quest") {
      u.startQuest(s.getFirstQuest());

      AND_WHEN("he right-clicks on the questgiver") {
        REPEAT_FOR_MS(100);
        a.onRightClick(c.client());

        THEN("it doesn't have a window") { CHECK(!a.window()); }
      }

      AND_WHEN("he kills the mouse") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(mouseSerial));
        WAIT_UNTIL(mouse->isDead());

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));

          THEN("he is not on the quest") { WAIT_UNTIL(u.numQuests() == 0); }
        }
      }
    }
  }
}

TEST_CASE("Quest chains", "[quests]") {
  GIVEN("a quest with another quest as a prerequisite") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
      <quest id="quest2" startsAt="A" endsAt="A">
        <prerequisite id="quest1" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});
    auto aSerial = s.getFirstObject().serial();
    WAIT_UNTIL(c.objects().size() == 1);
    auto &u = s.getFirstUser();

    THEN("the user can see only one quest available") {
      const auto &a = c.getFirstObject();
      CHECK(a.startsQuests().size() == 1);
    }

    WHEN("the user tries to accept the second quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest2", aSerial));

      THEN("he is not on a quest") {
        REPEAT_FOR_MS(100);
        CHECK(u.numQuests() == 0);
      }
    }

    WHEN("the user starts and finishes the first quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", aSerial));
      WAIT_UNTIL(u.numQuests() == 1);
      c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));
      WAIT_UNTIL(u.numQuests() == 0);

      AND_WHEN("he right-clicks on the object") {
        auto &obj = c.getFirstObject();
        REPEAT_FOR_MS(100);
        obj.onRightClick(c.client());

        THEN("it has a window") { CHECK(obj.window()); }
      }

      AND_WHEN("he tries to accept the second quest") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest2", aSerial));

        THEN("he is on a quest") { WAIT_UNTIL(u.numQuests() == 1); }
      }
    }
  }
}

TEST_CASE("Object window is updated with quest changes", "[quests][ui]") {
  GIVEN("an object that only starts one quest") {
    auto data = R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="quest1" startsAt="A" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    WHEN("a user opens the object window") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 1);
      auto &obj = c.getFirstObject();
      obj.onRightClick(c.client());

      AND_WHEN("he accepts the quest") {
        c->sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", serial));

        THEN("The object has no window") {
          REPEAT_FOR_MS(100);
          CHECK(!obj.window());
        }
      }
    }
  }

  GIVEN("an object that starts and ends a quest") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    WHEN("a user opens the object window") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 1);
      auto &obj = c.getFirstObject();
      obj.onRightClick(c.client());

      AND_WHEN("he accepts the quest") {
        c->sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", serial));

        AND_WHEN("he completes the quest") {
          c->sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

          THEN("The object has no window") {
            REPEAT_FOR_MS(100);
            CHECK(!obj.window());
          }
        }
      }
    }
  }
}

TEST_CASE("Quest progress is persistent", "[quests]") {
  auto data = R"(
    <objectType id="A" />
    <quest id="finishedQuest" startsAt="A" endsAt="A" />
  )";
  {
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);

    s.waitForUsers(1);
    auto &alice = s.getFirstUser();

    // Given Alice has finished a quest
    alice.startQuest(*s->findQuest("finishedQuest"));
    alice.completeQuest("finishedQuest");

    // When the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);

    // Then she has completed the quest
    auto c = TestClient::WithUsernameAndDataString("alice", data);
    s.waitForUsers(1);
    const auto &alice = s.getFirstUser();
    CHECK(alice.hasCompletedQuest("finishedQuest"));
  }
}

TEST_CASE("On login, client objects reflect already completed quests",
          "[quests]") {
  GIVEN("a simple quest and object") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("A", {10, 15});

    WHEN("Alice finishes the quest then disconnects") {
      {
        auto c = TestClient::WithUsernameAndDataString("alice", data);
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();
        const auto quest = s->findQuest("quest1");
        alice.startQuest(*quest);
        alice.completeQuest("quest1");
      }

      AND_WHEN("she logs in") {
        auto c = TestClient::WithUsernameAndDataString("alice", data);

        THEN("she knows that a questgiver object gives no quests") {
          WAIT_UNTIL(c.objects().size() == 1);
          const auto &obj = c.getFirstObject();
          WAIT_UNTIL(obj.startsQuests().size() == 0);
        }
      }
    }
  }
}
