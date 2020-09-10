#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Simple quest") {
  GIVEN("a quest starting at A and ending at B") {
    auto data = R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="questFromAToB" startsAt="A" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);

    const auto &a = s.addObject("A", {10, 15});
    const auto &b = s.addObject("B", {15, 10});

    WHEN("a  client accepts the quest from A") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("questFromAToB", a.serial()));

      THEN("he is on a quest") { WAIT_UNTIL(user.numQuests() == 1); }

      AND_WHEN("he completes the quest at B") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("questFromAToB", b.serial()));

        THEN("he is not on a quest") { WAIT_UNTIL(!user.numQuests() == 0); }
      }
    }
  }
}

TEST_CASE("Cases where a quest should not be accepted") {
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

TEST_CASE("Cases where a quest should not be completed") {
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

TEST_CASE("A user must be on a quest to complete it") {
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

TEST_CASE("Identical source and destination") {
  GIVEN("two quests that start at A and end at B") {
    auto data = R"(
    <objectType id="A" />
    <objectType id="B" />
    <quest id="quest1" startsAt="A" endsAt="B" />
    <quest id="quest2" startsAt="A" endsAt="B" />
  )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    const auto &a = s.addObject("A", {10, 15});
    const auto &b = s.addObject("B", {15, 10});

    WHEN("a client accepts both quests") {
      s.waitForUsers(1);
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", a.serial()));
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest2", a.serial()));

      THEN("he is on two quest") {
        auto &user = s.getFirstUser();
        WAIT_UNTIL(user.numQuests() == 2);

        AND_WHEN("he completes the quests at B") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", b.serial()));
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest2", b.serial()));

          THEN("he is not on any quests") {
            REPEAT_FOR_MS(100);
            CHECK(user.numQuests() == 0);
          }
        }
      }
    }
  }
}

TEST_CASE("Client knows about objects' quests") {
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

TEST_CASE("Clients know quests' correct end nodes") {
  GIVEN("a quest from A to B") {
    auto data = R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="quest1" startsAt="A" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);

    const auto &a = s.addObject("A", {10, 15});
    const auto &b = s.addObject("B", {15, 10});

    WHEN("a user accepts the quest") {
      auto c = TestClient::WithDataString(data);
      WAIT_UNTIL(c.objects().size() == 2);
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", a.serial()));

      THEN("he knows object B ends it") {
        auto &cB = c.objects()[b.serial()];
        REQUIRE(cB != nullptr);
        WAIT_UNTIL(cB->completableQuests().size() == 1);
      }
    }
  }
}

TEST_CASE("Client knows when quests can be completed") {
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

TEST_CASE("Client knows when objects have no quests") {
  GIVEN("an object B, with no quests") {
    auto data = R"(
      <objectType id="B" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addObject("B", {10, 15});

    WHEN("a client logs in") {
      auto c = TestClient::WithDataString(data);

      THEN("he knows that the object has no quests") {
        WAIT_UNTIL(c.objects().size() == 1);
        const auto &b = c.getFirstObject();
        REPEAT_FOR_MS(100);
        CHECK(b.startsQuests().empty());
      }
    }
  }
}

TEST_CASE("A user can't pick up a quest he's already on") {
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

TEST_CASE("After a user accepts a quest, he can't do so again") {
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

TEST_CASE("Quest UI", "[ui][.flaky]") {
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
        obj.onRightClick();
        WAIT_UNTIL(obj.window() != nullptr);

        auto questButtonE = obj.window()->findChild("quest1");
        auto questButton = dynamic_cast<Button *>(questButtonE);
        REQUIRE(questButton);

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

TEST_CASE("Quest UI for NPCs", "[ui][.flaky]") {
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
        npc.onRightClick();

        THEN("The object window is visible") {
          WAIT_UNTIL(npc.window() != nullptr);
          WAIT_UNTIL(npc.window()->visible());
        }
      }
    }
  }
}

TEST_CASE("Show the user when an object has no more quests") {
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

TEST_CASE("Kill quests") {
  GIVEN("A quest that requires a rat to be killed, and a rat") {
    auto data = R"(
      <objectType id="A" />
      <npcType id="rat" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective type="kill" id="rat" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);
    auto &u = s.getFirstUser();

    const auto &a = s.addObject("A", {10, 5});
    const auto &rat = s.addNPC("rat", {10, 15});

    WAIT_UNTIL(c.objects().size() == 2);
    const auto cA = c.objects()[a.serial()];

    WHEN("the user is on the quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", a.serial()));
      WAIT_UNTIL(u.numQuests() == 1);

      THEN("the user can see no completable quests at A") {
        REPEAT_FOR_MS(100);
        CHECK(cA->completableQuests().size() == 0);
      }

      AND_WHEN("the user tries to complete the quest") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));
        THEN("he is still on the quest") {
          REPEAT_FOR_MS(100);
          CHECK(u.numQuests() == 1);
        }
      }

      AND_WHEN("he kills a rat") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(rat.serial()));
        WAIT_UNTIL(rat.isDead());

        THEN("he can see a completable quest at A") {
          WAIT_UNTIL(cA->completableQuests().size() == 1);
        }

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));

          THEN("he is not on the quest") { WAIT_UNTIL(u.numQuests() == 0); }
        }
      }

      AND_WHEN("there is also a \"mouse\" NPC") {
        auto extraData = R"(
          <npcType id="mouse" />
        )";
        s.loadDataFromString(extraData);
        c.loadDataFromString(extraData);

        const auto &mouse = s.addNPC("mouse", {5, 10});

        AND_WHEN("the user kills the mouse") {
          c.sendMessage(CL_TARGET_ENTITY, makeArgs(mouse.serial()));
          WAIT_UNTIL(mouse.isDead());

          AND_WHEN("he tries to complete the quest") {
            c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));

            THEN("he is still on the quest") {
              REPEAT_FOR_MS(100);
              CHECK(u.numQuests() == 1);
            }
          }
        }
      }
    }

    WHEN("the user kills a rat while not on the quest") {
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(rat.serial()));
      WAIT_UNTIL(rat.isDead());
      REPEAT_FOR_MS(100);

      AND_WHEN("he starts the quest") {
        u.startQuest(s.getFirstQuest());

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));

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
        <objective type="kill" id="mouse" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);
    auto &u = s.getFirstUser();

    const auto &a = s.addObject("A", {10, 5});
    WAIT_UNTIL(c.objects().size() == 1);
    auto &cA = c.getFirstObject();

    const auto &mouse = s.addNPC("mouse", {10, 15});
    WAIT_UNTIL(c.objects().size() == 2);

    WHEN("the user is on the quest") {
      u.startQuest(s.getFirstQuest());

      AND_WHEN("he right-clicks on the questgiver") {
        REPEAT_FOR_MS(100);
        cA.onRightClick();

        THEN("it doesn't have a window") { CHECK(!cA.window()); }
      }

      AND_WHEN("he kills the mouse") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(mouse.serial()));
        WAIT_UNTIL(mouse.isDead());

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));

          THEN("he is not on the quest") { WAIT_UNTIL(u.numQuests() == 0); }
        }
      }
    }
  }

  GIVEN("A quest that requires two rats to be killed, and two rats") {
    auto data = R"(
      <objectType id="A" />
      <npcType id="rat" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective type="kill" id="rat" qty="2" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);
    auto &u = s.getFirstUser();

    const auto &a = s.addObject("A", {10, 5});
    const auto &rat1 = s.addNPC("rat", {10, 15});
    const auto &rat2 = s.addNPC("rat", {5, 10});
    WAIT_UNTIL(c.objects().size() == 3);
    const auto &cA = c.objects()[a.serial()];

    WHEN("the user accepts the quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", a.serial()));
      WAIT_UNTIL(u.numQuests() == 1);

      AND_WHEN("he kills a rat") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(rat1.serial()));
        WAIT_UNTIL(rat1.isDead());

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));

          THEN("he is still on the quest") {
            REPEAT_FOR_MS(100);
            CHECK(u.numQuests() == 1);
          }
        }

        AND_WHEN("he kills another rat") {
          c.sendMessage(CL_TARGET_ENTITY, makeArgs(rat2.serial()));
          WAIT_UNTIL_TIMEOUT(rat2.isDead(), 10000);

          AND_WHEN("he tries to complete the quest") {
            c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", a.serial()));

            THEN("he is not on the quest") { WAIT_UNTIL(u.numQuests() == 0); }
          }
        }
      }
    }
  }
}

TEST_CASE("Quest chains") {
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
        obj.onRightClick();

        THEN("it has a window") { CHECK(obj.window()); }
      }

      AND_WHEN("he tries to accept the second quest") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest2", aSerial));

        THEN("he is on a quest") { WAIT_UNTIL(u.numQuests() == 1); }
      }
    }
  }

  GIVEN("one quest that unlocks two others") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
      <quest id="quest2a" startsAt="A" endsAt="A">
        <prerequisite id="quest1" />
      </quest>
      <quest id="quest2b" startsAt="A" endsAt="A">
        <prerequisite id="quest1" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});
    auto aSerial = s.getFirstObject().serial();
    WAIT_UNTIL(c.objects().size() == 1);
    auto &user = s.getFirstUser();

    WHEN("the first quest is started and finished") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", aSerial));
      WAIT_UNTIL(user.numQuests() == 1);
      c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", aSerial));
      WAIT_UNTIL(user.numQuests() == 0);

      THEN("the user can see two quests available") {
        const auto &object = c.getFirstObject();
        WAIT_UNTIL(object.startsQuests().size() == 2);
      }
    }
  }

  GIVEN("a quest that has two prerequisite quests") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1a" startsAt="A" endsAt="A" />
      <quest id="quest1b" startsAt="A" endsAt="A" />
      <quest id="quest2" startsAt="A" endsAt="A">
        <prerequisite id="quest1a" />
        <prerequisite id="quest1b" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});
    auto aSerial = s.getFirstObject().serial();
    WAIT_UNTIL(c.objects().size() == 1);
    const auto &cObject = c.getFirstObject();
    auto &user = s.getFirstUser();

    WHEN("the prerequisite quests are started") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1a", aSerial));
      WAIT_UNTIL(user.numQuests() == 1);
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1b", aSerial));
      WAIT_UNTIL(user.numQuests() == 1);

      AND_WHEN("one is completed") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1a", aSerial));

        THEN("the user can see no quests available") {
          REPEAT_FOR_MS(100);
          CHECK(cObject.startsQuests().size() == 0);
        }

        AND_WHEN("the other is completed") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1b", aSerial));

          THEN("the user can see a quest available") {
            REPEAT_FOR_MS(100);
            CHECK(cObject.startsQuests().size() == 1);
          }
        }
      }
    }
  }
}

TEST_CASE("Object window stays open for chained quests", "[ui][.flaky]") {
  GIVEN("a quest chain") {
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
    WAIT_UNTIL(c.objects().size() == 1);
    auto &questgiver = c.getFirstObject();

    WHEN("a user starts the first quest") {
      auto &user = s.getFirstUser();
      user.startQuest(*s->findQuest("quest1"));

      AND_WHEN("he opens the questgiver's window") {
        questgiver.onRightClick();
        WAIT_UNTIL(questgiver.window() != nullptr);

        AND_WHEN("he completes the quest") {
          c.sendMessage(CL_COMPLETE_QUEST,
                        makeArgs("quest1", questgiver.serial()));

          THEN("the object window is still open") {
            REPEAT_FOR_MS(100);
            CHECK(questgiver.window() != nullptr);
          }
        }
      }
    }
  }
}

TEST_CASE("Object window is updated with quest changes", "[ui][.flaky]") {
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
      obj.onRightClick();
      WAIT_UNTIL(obj.window() != nullptr);

      AND_WHEN("he accepts the quest") {
        c->sendMessage({CL_ACCEPT_QUEST, makeArgs("quest1", serial)});

        THEN("The object has no window") {
          REPEAT_FOR_MS(100);
          CHECK(obj.window() == nullptr);
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
      obj.onRightClick();

      AND_WHEN("he accepts the quest") {
        c->sendMessage({CL_ACCEPT_QUEST, makeArgs("quest1", serial)});

        AND_WHEN("he completes the quest") {
          c->sendMessage({CL_COMPLETE_QUEST, makeArgs("quest1", serial)});

          THEN("The object has no window") {
            REPEAT_FOR_MS(100);
            CHECK(!obj.window());
          }
        }
      }
    }
  }
}

TEST_CASE("Quest progress is persistent") {
  auto data = R"(
    <objectType id="A" />
    <objectType id="B" />
    <item id="widget" />
    <quest id="startMe" startsAt="A" endsAt="A" />
    <quest id="leaveUnfinished" startsAt="A" endsAt="A">
      <objective type="kill" id="A" qty="10" />
    </quest>
    <quest id="makeNoProgress" startsAt="A" endsAt="A">
      <objective type="kill" id="B" />
    </quest>
    <quest id="killIt" startsAt="A" endsAt="A">
      <objective type="kill" id="A" />
    </quest>
    <quest id="buildIt" startsAt="A" endsAt="A">
      <objective type="construct" id="A" />
    </quest>
    <quest id="finishMe" startsAt="A" endsAt="A" />
    <quest id="hasPrereq" startsAt="A" endsAt="A">
      <prerequisite id="finishMe" />
    </quest>
    <quest id="fetchIt" startsAt="A" endsAt="A">
      <objective type="fetch" id="widget" />
    </quest>
  )";
  {
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);

    s.waitForUsers(1);
    auto &alice = s.getFirstUser();

    // Given that Alice has started quest A
    alice.startQuest(*s->findQuest("startMe"));

    // And started B, but not met its objective
    alice.startQuest(*s->findQuest("leaveUnfinished"));

    // And started C, but not made any progress
    alice.startQuest(*s->findQuest("makeNoProgress"));

    // And killed the target of D
    alice.startQuest(*s->findQuest("killIt"));
    alice.addQuestProgress(Quest::Objective::KILL, "A");

    // And constructed the target of E
    alice.startQuest(*s->findQuest("buildIt"));
    alice.addQuestProgress(Quest::Objective::CONSTRUCT, "A");

    // and finished F
    alice.startQuest(*s->findQuest("finishMe"));
    alice.completeQuest("finishMe");

    // and collected the target of G
    alice.startQuest(*s->findQuest("fetchIt"));
    alice.giveItem(&s.getFirstItem());

    // When the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);
    s.waitForUsers(1);
    const auto &alice = s.getFirstUser();

    // Then she is on A
    CHECK(alice.isOnQuest("startMe"));

    // And she knows she's on B
    const auto &leaveUnfinished =
        c->gameData.quests.find("leaveUnfinished")->second;
    WAIT_UNTIL(leaveUnfinished.state == CQuest::IN_PROGRESS);

    // And she knows she's on C
    const auto &makeNoProgress =
        c->gameData.quests.find("makeNoProgress")->second;
    WAIT_UNTIL(makeNoProgress.state == CQuest::IN_PROGRESS);

    // And has achieved the objective of D
    auto killIt = s->findQuest("killIt");
    CHECK(killIt->canBeCompletedByUser(alice));

    // And has achieved the objective of E
    auto buidlIt = s->findQuest("buildIt");
    CHECK(buidlIt->canBeCompletedByUser(alice));

    // And completed F
    CHECK(alice.hasCompletedQuest("finishMe"));

    // And can see the quest that has F as a prerequisite
    s.addObject("A", {10, 15});
    WAIT_UNTIL(c.objects().size() == 1);
    const auto &cObj = c.getFirstObject();
    REPEAT_FOR_MS(100);
    CHECK(cObj.startsQuests().size() == 1);

    // And has achieved the objective of G
    auto fetchIt = s->findQuest("fetchIt");
    CHECK(fetchIt->canBeCompletedByUser(alice));
  }
}

TEST_CASE("Clients get the correct state on login") {
  GIVEN("two quests, and a questgiver") {
    auto data = R"(
      <objectType id="questgiver" />
      <objectType id="B" />
      <quest id="partial" startsAt="questgiver" endsAt="B">
        <objective type="kill" id="B" qty="2" />
      </quest>
      <quest id="completable" startsAt="questgiver" endsAt="B" />
      <quest id="completed" startsAt="questgiver" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("questgiver", {10, 15});

    WHEN(
        "Alice starts one quest, partially finishes another, finishes "
        "another, and disconnects") {
      {
        auto c = TestClient::WithUsernameAndDataString("alice", data);
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();

        alice.startQuest(*s->findQuest("partial"));
        alice.addQuestProgress(Quest::Objective::KILL, "B");

        alice.startQuest(*s->findQuest("completable"));

        alice.startQuest(*s->findQuest("completed"));
        alice.completeQuest("completed");
      }

      AND_WHEN("she logs in") {
        auto c = TestClient::WithUsernameAndDataString("alice", data);

        THEN("she knows that the questgiver gives no quests") {
          WAIT_UNTIL(c.objects().size() == 1);
          const auto &obj = c.getFirstObject();
          WAIT_UNTIL(obj.startsQuests().size() == 0);

          AND_THEN("she knows that she's killed 1/2 targets for 'partial'") {
            const auto &partialQuest =
                c->gameData.quests.find("partial")->second;
            WAIT_UNTIL(partialQuest.getProgress(0) == 1);
          }

          AND_THEN("she knows that she is on 'completable'") {
            const auto &questInProgress =
                c->gameData.quests.find("completable")->second;
            WAIT_UNTIL(questInProgress.state == CQuest::CAN_FINISH);
          }
        }
      }
    }
  }
}

TEST_CASE("Quests give XP") {
  GIVEN("A quest and a user") {
    auto data = R"(
      <objectType id="A" />
      <quest id="quest1" startsAt="A" endsAt="A" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("he completes the quest") {
      user.startQuest(*s->findQuest("quest1"));
      user.completeQuest("quest1");

      THEN("he has XP") { CHECK(user.xp() > 0); }
    }
  }
}

TEST_CASE("Fetch quests") {
  GIVEN("a fetch quest") {
    auto data = R"(
      <objectType id="A" />
      <item id="eyeball" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective id="eyeball" type="fetch" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    auto &eyeball = s.getFirstItem();

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("a user accepts the quest") {
      user.startQuest(*s->findQuest("quest1"));

      AND_WHEN("he gets the item") {
        user.giveItem(&eyeball);

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

          THEN("he has completed the quest") {
            WAIT_UNTIL(user.hasCompletedQuest("quest1"));

            AND_THEN("he no longer has the item") {
              auto itemSet = ItemSet{};
              itemSet.add(&eyeball);
              CHECK_FALSE(user.hasItems(itemSet));
            }
          }
        }
      }
    }

    WHEN("a user has the item already") {
      user.giveItem(&eyeball);

      AND_WHEN("he accepts the quest") {
        user.startQuest(*s->findQuest("quest1"));

        THEN("he knows he can complete it") {
          const auto &cQuest = c.getFirstQuest();
          WAIT_UNTIL(cQuest.state == CQuest::CAN_FINISH);
        }
      }
    }
  }

  GIVEN("A user on a quest to get two eyeballs") {
    auto data = R"(
      <objectType id="A" />
      <item id="eyeball" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective id="eyeball" type="fetch" qty="2" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(*s->findQuest("quest1"));

    WHEN("he gets one") {
      auto &eyeball = s.getFirstItem();
      user.giveItem(&eyeball);

      AND_WHEN("he tries to complete the quest") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

        THEN("he is still on the quest") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user.hasCompletedQuest("quest1"));
        }
      }
    }
  }

  GIVEN("A user on a quest to get two eyeballs and a nose") {
    auto data = R"(
      <objectType id="A" />
      <item id="eyeball" />
      <item id="nose" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective type="fetch" id="eyeball" qty="2" />
        <objective type="fetch" id="nose" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(*s->findQuest("quest1"));

    WHEN("he gets the eyeballs") {
      auto eyeball = s->findItem("eyeball");
      user.giveItem(eyeball, 2);

      AND_WHEN("he tries to complete the quest") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

        THEN("he is still on the quest") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user.hasCompletedQuest("quest1"));
        }
      }

      AND_WHEN("he gets the nose") {
        auto nose = s->findItem("nose");
        user.giveItem(nose, 1);

        AND_WHEN("he tries to complete the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

          THEN("he has completed the quest") {
            WAIT_UNTIL(user.hasCompletedQuest("quest1"));

            AND_THEN("all items are removed from his inventory") {
              auto eyeballAsSet = ItemSet{};
              eyeballAsSet.add(eyeball);
              CHECK_FALSE(user.hasItems(eyeballAsSet));
            }
          }
        }
      }
    }
  }

  GIVEN("a quest to get a craftable breath") {
    auto data = R"(
      <objectType id="questgiver" />
      <item id="breath" />
      <recipe id="breath" time="1" />
      <quest id="breatheOnMe" startsAt="questgiver" endsAt="questgiver">
        <objective type="fetch" id="breath" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    /*s.addObject("questgiver", {10, 15});
    auto serial = s.getFirstObject().serial();*/

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(s.getFirstQuest());

    WHEN("the user creates a breath") {
      c.sendMessage(CL_CRAFT, "breath");

      THEN("he knows that he can complete the quest") {
        const auto &quest = c.getFirstQuest();
        WAIT_UNTIL(quest.state == CQuest::CAN_FINISH);
      }
    }
  }
}

TEST_CASE("Multiple objectives", "[!mayfail]") {
  GIVEN("A user on a fetch quest for two items") {
    auto data = R"(
      <objectType id="A" />
      <item id="leftPiece" />
      <item id="rightPiece" />
      <quest id="quest1" startsAt="A" endsAt="A">
        <objective type="fetch" id="leftPiece" />
        <objective type="fetch" id="rightPiece" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});
    auto serial = s.getFirstObject().serial();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(*s->findQuest("quest1"));

    const auto &quest = s.getFirstQuest();

    WHEN("he gets the first item") {
      auto &leftPiece = *s.items().find(ServerItem{"leftPiece"});
      user.giveItem(&leftPiece);

      THEN("he can't complete the quest") {
        CHECK_FALSE(quest.canBeCompletedByUser(user));
      }
    }
  }
}

TEST_CASE("Quest items that drop only while on quest") {
  GIVEN("A quest for a dragon's scale") {
    auto data = R"(
      <objectType id="questgiver" />
      <npcType id="dragon">
        <loot id="scale" />
      </npcType>
      <item id="scale" exclusiveToQuest="quest1" />
      <quest id="quest1" startsAt="questgiver" endsAt="questgiver">
        <objective type="fetch" id="scale" />
      </quest>
      <quest id="quest2" startsAt="questgiver" endsAt="questgiver" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    s.addNPC("dragon", {10, 15});
    WAIT_UNTIL(c.objects().size() == 1);
    const auto &dragon = c.getFirstNPC();

    WHEN("a player kills a dragon") {
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(dragon.serial()));
      WAIT_UNTIL(dragon.isDead());

      THEN("it has no loot") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(dragon.lootable());
      }
    }

    WHEN("a player is on the quest") {
      user.startQuest(*s->findQuest("quest1"));

      AND_WHEN("he kills a dragon") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(dragon.serial()));
        WAIT_UNTIL(dragon.isDead());

        THEN("it has loot") {
          WAIT_UNTIL(dragon.lootable());

          AND_WHEN("he loots it") {
            c.sendMessage(CL_TAKE_ITEM, makeArgs(dragon.serial(), 0));
            WAIT_UNTIL(c.inventory()[0].first.type() != nullptr);

            AND_WHEN("he kills another dragon") {
              const auto &dragon2 = s.addNPC("dragon", {10, 5});
              WAIT_UNTIL(c.objects().size() == 2);
              auto it = c.objects().find(dragon2.serial());
              const auto &cDragon2 = *dynamic_cast<ClientNPC *>(it->second);

              c.sendMessage(CL_TARGET_ENTITY, makeArgs(dragon2.serial()));
              WAIT_UNTIL(dragon2.isDead());

              THEN("it has no loot") {
                REPEAT_FOR_MS(100);
                CHECK_FALSE(cDragon2.lootable());
              }
            }

            AND_WHEN("he abandons the quest") {
              c.sendMessage(CL_ABANDON_QUEST, "quest1");

              THEN("he doesn't have the scale any more") {
                WAIT_UNTIL(!user.inventory(0).first.hasItem());
              }
            }
          }
        }
      }
    }

    WHEN("a player is on a different quest") {
      user.startQuest(*s->findQuest("quest2"));

      AND_WHEN("he kills a dragon") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(dragon.serial()));
        WAIT_UNTIL(dragon.isDead());

        THEN("it has no loot") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(dragon.lootable());
        }
      }
    }
  }
}

TEST_CASE("Construction quests") {
  GIVEN("a quest to construct a chair") {
    auto data = R"(
      <objectType id="questgiver" />
      <objectType id="chair" constructionTime="0" >
        <material id="wood" />
      </objectType>
      <item id="wood" />
      <quest id="quest1" startsAt="questgiver" endsAt="questgiver">
        <objective type="construct" id="chair" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);  // Ensures object serials are sequential

    s.addObject("questgiver", {10, 15});
    WAIT_UNTIL(c.objects().size() == 1);
    auto serial = c.getFirstObject().serial();
    auto &user = s.getFirstUser();

    WHEN("a user starts the quest") {
      user.startQuest(*s->findQuest("quest1"));

      THEN("the client knows it isn't completable") {
        REPEAT_FOR_MS(100);
        const auto &quest1 = c->gameData.quests.find("quest1")->second;
        CHECK(quest1.state == CQuest::IN_PROGRESS);
        const auto &cQuestgiver = c.getFirstObject();
        CHECK(cQuestgiver.completableQuests().empty());
      }

      AND_WHEN("he tries to finish it") {
        c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

        THEN("he is still on the quest") {
          REPEAT_FOR_MS(100);
          CHECK(user.isOnQuest("quest1"));
        }
      }

      AND_WHEN("he constructs a chair") {
        const auto &chair = s.addObject("chair", {5, 10}, user.name());
        WAIT_UNTIL(c.objects().size() == 2);

        auto &wood = s.getFirstItem();
        user.giveItem(&wood);
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Serial::Inventory(), 0, chair.serial(), 0));
        WAIT_UNTIL(!chair.isBeingBuilt());

        AND_WHEN("he tries to finish the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

          THEN("he is has completed it") {
            WAIT_UNTIL(user.hasCompletedQuest("quest1"));
          }
        }
      }
    }
  }

  GIVEN("a quest to construct a washer and dryer") {
    auto data = R"(
      <objectType id="questgiver" />
      <objectType id="washer" constructionTime="0" >
        <material id="parts" />
      </objectType>
      <objectType id="dryer" constructionTime="0" >
        <material id="parts" />
      </objectType>
      <item id="parts" />
      <quest id="quest1" startsAt="questgiver" endsAt="questgiver">
        <objective type="construct" id="washer" />
        <objective type="construct" id="dryer" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    WAIT_UNTIL(s.users().size() == 1);  // Ensures object serials are sequential

    s.addObject("questgiver", {10, 15});
    WAIT_UNTIL(c.objects().size() == 1);
    auto serial = c.getFirstObject().serial();
    auto &user = s.getFirstUser();

    WHEN("a user starts the quest") {
      user.startQuest(*s->findQuest("quest1"));

      AND_WHEN("he constructs a dryer") {
        const auto &dryer = s.addObject("dryer", {5, 10}, user.name());
        WAIT_UNTIL(c.objects().size() == 2);

        auto &parts = s.getFirstItem();
        user.giveItem(&parts, 2);
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Serial::Inventory(), 0, dryer.serial(), 0));
        WAIT_UNTIL(!dryer.isBeingBuilt());

        AND_WHEN("he tries to finish the quest") {
          c.sendMessage(CL_COMPLETE_QUEST, makeArgs("quest1", serial));

          THEN("he has not completed it") {
            REPEAT_FOR_MS(100);
            CHECK_FALSE(user.hasCompletedQuest("quest1"));
          }
        }
      }
    }
  }
}

TEST_CASE("Quests that give items when you start") {
  GIVEN("a questgiver, and a variety of quests") {
    auto data = R"(
      <objectType id="questgiver" />
      <item id="key" />
      <item id="gun" />
      <quest id="openTheDoor" startsAt="questgiver" endsAt="questgiver">
        <startsWithItem id="key" />
      </quest>
      <quest id="assassinate" startsAt="questgiver" endsAt="questgiver">
        <startsWithItem id="gun" />
      </quest>
      <quest id="robHouse" startsAt="questgiver" endsAt="questgiver">
        <startsWithItem id="key" />
        <startsWithItem id="gun" />
      </quest>
      <quest id="doNothing" startsAt="questgiver" endsAt="questgiver" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("questgiver", {10, 15});
    WAIT_UNTIL(c.objects().size() == 1);
    auto serial = c.getFirstObject().serial();
    auto &user = s.getFirstUser();

    WHEN("a user accepts a quest that gives a key") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("openTheDoor", serial));

      THEN("the user has a key") {
        const auto &firstSlot = user.inventory()[0];
        WAIT_UNTIL(firstSlot.first.hasItem());
      }
    }

    WHEN("a user accepts a minimal quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("doNothing", serial));

      THEN("the user has no items") {
        REPEAT_FOR_MS(100);
        const auto &firstSlot = user.inventory()[0];
        CHECK_FALSE(firstSlot.first.hasItem());
      }
    }

    WHEN("a user accepts two quests that grant two different items") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("openTheDoor", serial));
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("assassinate", serial));

      THEN("the user has both items") {
        auto requiredItems = ItemSet{};
        requiredItems.add(s->findItem("key"));
        requiredItems.add(s->findItem("gun"));
        REPEAT_FOR_MS(100);

        WAIT_UNTIL(user.hasItems(requiredItems));
      }
    }

    WHEN("a user accepts a quest that gives two items") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("robHouse", serial));

      THEN("the user has both items") {
        auto requiredItems = ItemSet{};
        requiredItems.add(s->findItem("key"));
        requiredItems.add(s->findItem("gun"));
        REPEAT_FOR_MS(100);

        WAIT_UNTIL(user.hasItems(requiredItems));
      }
    }

    WHEN("a user has a full inventory") {
      for (auto i = 0; i != User::INVENTORY_SIZE; ++i)
        user.giveItem(&s.getFirstItem());
      AND_WHEN("he tries to accept an item-granting quest") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("openTheDoor", serial));
        THEN("he is not on a quest") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user.isOnQuest("openTheDoor"));
        }
      }
    }
  }
}

TEST_CASE("Quest reward: construction") {
  GIVEN("a quest that awards a construction project") {
    auto data = R"(
      <objectType id="questgiver" />
      <item id="air" />
      <objectType id="hole">
        <material id="air" />
        <unlockedBy />
      </objectType>
      <item id="key" />
      <quest id="teachesHole" startsAt="questgiver" endsAt="questgiver">
        <reward type="construction" id="hole" />
      </quest>
      <spell id="hole" range=30 cooldown=2 >
        <targets enemy=1 />
        <function name="doDirectDamage" />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("a user completes the quest") {
      const auto &quest = s.getFirstQuest();
      user.startQuest(quest);
      user.completeQuest(quest.id);

      THEN("he knows the construction project") {
        WAIT_UNTIL(user.knowsConstruction("hole"));

        AND_THEN("and he knows it") { WAIT_UNTIL(c.knowsConstruction("hole")); }

        AND_THEN("he doesn't know the spell of the same name") {
          CHECK_FALSE(user.getClass().knowsSpell("hole"));
        }
      }
    }
  }
}

TEST_CASE("Quest reward: spell") {
  GIVEN("a quest that awards a spell") {
    auto data = R"(
      <objectType id="questgiver" />
      <spell id="fireball" range=30 cooldown=2 >
        <targets enemy=1 />
        <function name="doDirectDamage" />
      </spell>
      <quest id="teachesFireball" startsAt="questgiver" endsAt="questgiver">
        <reward type="spell" id="fireball" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    THEN("the user doesn't know any spells") {
      CHECK_FALSE(user.getClass().knowsSpell("fireball"));
    }

    WHEN("a user completes the quest") {
      const auto &quest = s.getFirstQuest();
      user.startQuest(quest);
      user.completeQuest(quest.id);

      THEN("he knows the spell") {
        WAIT_UNTIL(user.getClass().knowsSpell("fireball"));
      }
    }
  }
}

TEST_CASE("Quest reward: item") {
  GIVEN("a quest that awards an item") {
    auto data = R"(
      <objectType id="questgiver" />
      <item id="gold" />
      <quest id="givesGold" startsAt="questgiver" endsAt="questgiver">
        <reward type="item" id="gold" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto &quest = s.getFirstQuest();
    user.startQuest(quest);

    WHEN("a user completes the quest") {
      user.completeQuest(quest.id);

      THEN("he has an inventory item") {
        WAIT_UNTIL(user.inventory(0).first.hasItem());
      }
    }

    WHEN("a user's inventory is full") {
      const auto &filler = s.getFirstItem();
      user.giveItem(&filler, User::INVENTORY_SIZE);

      AND_WHEN("he tries to complete the quest") {
        user.completeQuest(quest.id);

        THEN("he is still on a quest") {
          REPEAT_FOR_MS(100);
          CHECK(user.numQuests() == 1);
        }
      }
    }
  }

  GIVEN("a quest that rewards two items") {
    auto data = R"(
      <objectType id="questgiver" />
      <item id="gold" stackSize="2" />
      <quest id="givesGold" startsAt="questgiver" endsAt="questgiver">
        <reward type="item" id="gold" qty="2" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto &quest = s.getFirstQuest();
    user.startQuest(quest);

    WHEN("a user completes the quest") {
      const auto &quest = s.getFirstQuest();
      user.startQuest(quest);
      user.completeQuest(quest.id);

      THEN("he has two of the items") { CHECK(user.inventory(0).second == 2); }
    }
  }

  GIVEN("a quest that awards a nonexistent item") {
    auto data = R"(
      <item id="filler" />
      <objectType id="questgiver" />
      <quest id="givesGold" startsAt="questgiver" endsAt="questgiver">
        <reward type="item" id="fakeName" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    const auto &quest = s.getFirstQuest();
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(quest);

    WHEN("a user completes the quest") {
      user.completeQuest(quest.id);

      THEN("the server doesn't crash") { s.nop(); }
    }
  }
}

TEST_CASE("Multiple quest rewards") {
  GIVEN("a quest that awards two different items") {
    auto data = R"(
      <objectType id="questgiver" />
      <item id="strawberry" />
      <item id="grape" />
      <quest id="giveFruits" startsAt="questgiver" endsAt="questgiver">
        <reward type="item" id="strawberry" />
        <reward type="item" id="grape" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("a user completes the quest") {
      const auto &quest = s.getFirstQuest();
      user.startQuest(quest);
      user.completeQuest(quest.id);

      THEN("he has two items") {
        WAIT_UNTIL(user.inventory(0).first.hasItem());
        WAIT_UNTIL(user.inventory(1).first.hasItem());
      }
    }
  }
}

TEST_CASE("Client remembers quest progress after death") {
  GIVEN("a player on a quest, and a quest-starter object") {
    auto data = R"(
      <objectType id="questStarter" />
      <objectType id="questEnder" />
      <quest id="quest1" startsAt="questStarter" endsAt="questEnder" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("questStarter", {10, 15});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(s.getFirstQuest());

    WHEN("he dies") {
      REPEAT_FOR_MS(100);
      user.kill();

      THEN("he knows he's on the quest") {
        REPEAT_FOR_MS(100);
        const auto &quest = c.getFirstQuest();
        CHECK(quest.state == CQuest::CAN_FINISH);
      }
    }
  }
}

TEST_CASE("Abandoning quests") {
  auto data = R"(
    <objectType id="questgiver" />
    <quest id="quest1" startsAt="questgiver" endsAt="questgiver" />
    <quest id="quest2" startsAt="questgiver" endsAt="questgiver" />
  )";
  auto s = TestServer::WithDataString(data);
  auto c = TestClient::WithDataString(data);

  GIVEN("a player on a quest") {
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(s.findQuest("quest1"));

    WHEN("he tries to abandon the quest") {
      c.sendMessage(CL_ABANDON_QUEST, "quest1");
      THEN("he is not on any quests") {
        WAIT_UNTIL(user.questsInProgress().empty());
        AND_THEN("he knows it") {
          const auto &quest = c.getFirstQuest();
          WAIT_UNTIL(quest.state == CQuest::CAN_START);
        }
      }
    }
  }

  GIVEN("a player on two quest") {
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.startQuest(s.findQuest("quest1"));
    user.startQuest(s.findQuest("quest2"));

    WHEN("he tries to abandon one") {
      c.sendMessage(CL_ABANDON_QUEST, "quest1");
      REPEAT_FOR_MS(100);
      THEN("he is on one quest") { CHECK(user.questsInProgress().size() == 1); }
    }
  }
}

// Sometimes causes a failed assertion: bad state when removing/inserting
// entities in Server::_entitiesBy?
TEST_CASE("Class-specific quests", "[.flaky]") {
  GIVEN("a quest marked as exclusive to a 'politician' class") {
    auto data = R"(
      <class name="politician" />
      <objectType id="questgiver" />
      <quest id="getElected" startsAt="questgiver" endsAt="questgiver"
  exclusiveToClass="politician" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addObject("questgiver", {10, 15});
    auto questgiver = s.getFirstObject().serial();

    THEN("the quest exists") { CHECK(s->findQuest("getElected")); }

    WHEN("a non-politician user logs in") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);

      THEN("he knows that the questgiver gives no quests") {
        WAIT_UNTIL(c.objects().size() == 1);
        const auto &obj = c.getFirstObject();
        REPEAT_FOR_MS(100);
        CHECK(obj.startsQuests().size() == 0);
      }

      AND_WHEN("he tries to accept it") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("getElected", questgiver));

        THEN("he isn't on any quests") {
          REPEAT_FOR_MS(100);
          auto &user = s.getFirstUser();
          CHECK(user.numQuests() == 0);
        }
      }
    }

    WHEN("a politician user logs in") {
      auto c = TestClient::WithClassAndDataString("politician", data);
      s.waitForUsers(1);

      THEN("he knows that the questgiver gives a quest") {
        WAIT_UNTIL(c.objects().size() == 1);
        const auto &obj = c.getFirstObject();
        WAIT_UNTIL(obj.startsQuests().size() == 1);
      }

      AND_WHEN("he tries to accept it") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("getElected", questgiver));

        THEN("he is on a quest") {
          auto &user = s.getFirstUser();
          WAIT_UNTIL(user.numQuests() == 1);
        }
      }
    }
  }

  GIVEN("a quest marked as exclusive to a 'frog' class") {
    auto data = R"(
      <class name="frog" />
      <objectType id="questgiver" />
      <quest id="ribbit" startsAt="questgiver" endsAt="questgiver"
        exclusiveToClass="frog" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addObject("questgiver", {10, 15});
    auto questgiver = s.getFirstObject().serial();

    WHEN("a frog user tries to accept it") {
      auto c = TestClient::WithClassAndDataString("frog", data);
      s.waitForUsers(1);
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("ribbit", questgiver));

      THEN("he is on a quest") {
        auto &user = s.getFirstUser();
        WAIT_UNTIL(user.numQuests() == 1);
      }
    }
  }
}

TEST_CASE("A class-specific quests with a prerequisite") {
  GIVEN("A police-exclusive quest, that requires going to school") {
    auto data = R"(
      <class name="police" />
      <objectType id="questgiver" />
      <quest id="arrestCriminals" startsAt="questgiver" endsAt="questgiver"
        exclusiveToClass="police">
        <prerequisite id="goToSchool" />
      </quest>
      <quest id="goToSchool" startsAt="questgiver" endsAt="questgiver" />
    )";

    WHEN("a non-police player logs in") {
      auto s = TestServer::WithDataString(data);
      auto c = TestClient::WithClassAndDataString("frog", data);
      s.addObject("questgiver", {10, 15});
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      CHECK(user.getClass().type().id() != "police");

      AND_WHEN("he goes to school") {
        const auto &goToSchool = s.findQuest("goToSchool");
        user.startQuest(goToSchool);
        user.completeQuest(goToSchool.id);

        THEN("he sees no quests at the questgiver") {
          WAIT_UNTIL(c.objects().size() == 1);
          const auto &obj = c.getFirstObject();
          REPEAT_FOR_MS(100);
          CHECK(obj.startsQuests().size() == 0);
        }
      }
    }
  }
}

TEST_CASE("Quest-exclusive objects") {
  GIVEN("a quest-exclusive tree that gives an acorn") {
    auto data = R"(
      <objectType id="tree" exclusiveToQuest="getAcorn">
        <yield id="acorn" />
      </objectType>
      <item id="acorn" />
      <quest id="getAcorn" startsAt="tree" endsAt="tree" />
      <quest id="differentQuest" startsAt="tree" endsAt="tree" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    const auto &user = s.getFirstUser();

    s.addObject("tree", {10, 15});
    const auto &tree = s.getFirstObject();

    WHEN("a user tries to gather from it") {
      c.sendMessage(CL_GATHER, makeArgs(tree.serial()));

      THEN("he doesn't have an item") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(user.inventory(0).first.hasItem());
      }
    }

    WHEN("a user starts the quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("getAcorn", tree.serial()));

      AND_WHEN("he tries to gather from it") {
        c.sendMessage(CL_GATHER, makeArgs(tree.serial()));

        THEN("he has an item") {
          WAIT_UNTIL(user.inventory(0).first.hasItem());
        }
      }
    }

    WHEN("a user starts a different quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("differentQuest", tree.serial()));

      AND_WHEN("he tries to gather from it") {
        c.sendMessage(CL_GATHER, makeArgs(tree.serial()));

        THEN("he doesn't have an item") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(user.inventory(0).first.hasItem());
        }
      }
    }
  }
}

TEST_CASE("Quest with a time limit") {
  GIVEN("a quest with a time limit of 1s") {
    auto data = R"(
      <objectType id="questgiver" />
      <quest id="q1" startsAt="questgiver" endsAt="questgiver" timeLimit="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    s.addObject("questgiver", {10, 15});
    const auto &questgiver = s.getFirstObject();

    WHEN("a user starts the quest") {
      c.sendMessage(CL_ACCEPT_QUEST, makeArgs("q1", questgiver.serial()));
      REPEAT_FOR_MS(500);
      CHECK(user.questsInProgress().size() == 1);

      AND_WHEN("1.1s elapses") {
        REPEAT_FOR_MS(1100);

        THEN("he is not on a quest") { CHECK(user.questsInProgress().empty()); }
      }
    }
  }
}

TEST_CASE("Quest time remaining is persistent") {
  // Given a quest with a time limit of 1s
  auto data = R"(
      <objectType id="questgiver" />
      <quest id="q1" startsAt="questgiver" endsAt="questgiver" timeLimit="1" />
    )";
  auto s = TestServer::WithDataString(data);
  s.addObject("questgiver", {10, 15});
  const auto &questgiver = s.getFirstObject();

  // When Alice connects
  {
    auto c = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);

    // And when she starts the quest
    c.sendMessage(CL_ACCEPT_QUEST, makeArgs("q1", questgiver.serial()));

    // And when she disconnects and reconnects
  }
  {
    auto c = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);
    auto &alice = s.getFirstUser();

    // And 0.5s elapse
    REPEAT_FOR_MS(500);

    // Then she is on a quest
    CHECK_FALSE(alice.questsInProgress().empty());

    // And when 0.6s elapse, enough for the quest to expire
    REPEAT_FOR_MS(600);

    // Then she is not on a quest
    CHECK(alice.questsInProgress().empty());
  }
}

TEST_CASE("Quest objective: cast a spell") {
  GIVEN("a quest to cast a spell") {
    auto data = R"(
      <objectType id="questgiver" />
      <spell id="fireball" range=30 >
        <targets self=1 />
        <function name="doDirectDamage" />
      </spell>
      <spell id="iceball" range=30 >
        <targets self=1 />
        <function name="doDirectDamage" />
      </spell>
      <quest id="castAFireball" startsAt="questgiver" endsAt="questgiver">
        <objective type="cast" id="fireball" />
      </quest>
    )";
    auto s = TestServer::WithDataString(data);

    s.addObject("questgiver", {10, 15});
    const auto &questgiver = s.getFirstObject();

    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.getClass().teachSpell("fireball");

    WHEN("a user accepts it") {
      c.sendMessage(CL_ACCEPT_QUEST,
                    makeArgs("castAFireball", questgiver.serial()));
      REPEAT_FOR_MS(100);

      AND_WHEN("he tries to complete it") {
        c.sendMessage(CL_COMPLETE_QUEST,
                      makeArgs("castAFireball", questgiver.serial()));
        REPEAT_FOR_MS(100);

        THEN("he is still on a quest") {
          CHECK(user.questsInProgress().size() == 1);
        }
      }

      AND_WHEN("he casts the spell") {
        c.sendMessage(CL_CAST, "fireball");

        AND_WHEN("he tries to complete it") {
          c.sendMessage(CL_COMPLETE_QUEST,
                        makeArgs("castAFireball", questgiver.serial()));
          REPEAT_FOR_MS(100);

          THEN("he is not on a quest") {
            WAIT_UNTIL(user.questsInProgress().size() == 0);
          }
        }
      }

      AND_WHEN("he casts a different spell") {
        c.sendMessage(CL_CAST, "iceball");

        AND_WHEN("he tries to complete it") {
          c.sendMessage(CL_COMPLETE_QUEST,
                        makeArgs("castAFireball", questgiver.serial()));
          REPEAT_FOR_MS(100);

          THEN("he is still on a quest") {
            CHECK(user.questsInProgress().size() == 1);
          }
        }
      }
    }
  }
}
