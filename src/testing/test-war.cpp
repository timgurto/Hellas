#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Basic declaration of war", "[war]") {
  // Given Alice is logged in
  TestServer s;
  TestClient alice = TestClient::WithUsername("Alice");
  s.waitForUsers(1);

  // When Alice sends a CL_DECLARE_WAR_ON_PLAYER message
  alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

  // Then Alice and Bob go to war
  WAIT_UNTIL(s.wars().isAtWar("Alice", "Bob"));
}

TEST_CASE("No erroneous wars", "[war]") {
  // When a clean server is started
  TestServer s;

  // Then Alice and Bob are not at war
  CHECK_FALSE(s.wars().isAtWar("Alice", "Bob"));
}

TEST_CASE("Wars are persistent", "[persistence][war]") {
  // Given Alice and Bob are at war, and there is no server running
  {
    TestServer server1;
    server1.wars().declare("Alice", "Bob");
  }

  // When a server begins that keeps persistent data
  TestServer server2 = TestServer::KeepingOldData();

  // Then Alice and Bob are still at war
  CHECK(server2.wars().isAtWar("Alice", "Bob"));
}

TEST_CASE("Clients are alerted of new wars", "[war]") {
  // Given Alice and Bob are logged in
  TestServer s;
  auto alice = TestClient::WithUsername("Alice");
  auto bob = TestClient::WithUsername("Bob");
  s.waitForUsers(2);

  // When Alice declares war on Bob
  alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

  // Then Alice is alerted to the new war
  WAIT_UNTIL(alice.otherUsers().size() == 1);
  auto &uBob = alice.getFirstOtherUser();
  WAIT_UNTIL(alice->isAtWarWith(uBob));
}

TEST_CASE("Clients are told of existing wars on login", "[war]") {
  // Given Alice and Bob are at war
  TestServer s;
  s.wars().declare("Alice", "Bob");

  // When Alice and Bob log in
  auto alice = TestClient::WithUsername("Alice");
  auto bob = TestClient::WithUsername("Bob");
  s.waitForUsers(2);

  // Then she is told about the war
  WAIT_UNTIL(alice.otherUsers().size() == 1);
  auto &uBob = alice.getFirstOtherUser();
  WAIT_UNTIL(alice->isAtWarWith(uBob));
}

TEST_CASE("Wars cannot be redeclared", "[war]") {
  // Given Alice and Bob are at war, and Alice is logged in
  TestServer s;
  TestClient alice = TestClient::WithUsername("Alice");
  s.wars().declare("Alice", "Bob");
  s.waitForUsers(1);

  // When Alice declares war on Bob
  alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

  // Then she receives an ERROR_ALREADY_AT_WAR error message
  CHECK(alice.waitForMessage(ERROR_ALREADY_AT_WAR));
}

TEST_CASE("A player can be at war with a city", "[city][war]") {
  // Given a running server;
  TestServer s;

  // And a city named Athens;
  s.cities().createCity("Athens", {}, {});

  // And a user named Alice
  TestClient c = TestClient::WithUsername("Alice");
  s.waitForUsers(1);

  // When a war is declared between Alice and Athens
  s.wars().declare("Alice", "Athens");

  // Then they are considered to be at war.
  CHECK(s.wars().isAtWar("Alice", "Athens"));
}

TEST_CASE("A player at war with a city is at war with its members",
          "[city][war]") {
  // Given a running server;
  TestServer s;

  // And a city named Athens;
  s.cities().createCity("Athens", {}, {});

  // And a user, Alice, who is a member of Athens;
  auto alice = TestClient::WithUsername("Alice");
  s.waitForUsers(1);
  s.cities().addPlayerToCity(s.getFirstUser(), "Athens");

  // When new user Bob and Athens go to war
  Belligerent b1("Bob", Belligerent::PLAYER), b2("Athens", Belligerent::CITY);

  SECTION("Bob logs in, then war is declared") {
    TestClient bob = TestClient::WithUsername("Bob");
    s.waitForUsers(2);
    s.wars().declare(b1, b2);

    // Then Bob is at war with Alice;
    CHECK(s.wars().isAtWar("Alice", "Bob"));

    // And Bob knows that he's at war with Alice
    WAIT_UNTIL(bob.otherUsers().size() == 1);
    const auto &cAlice = bob.getFirstOtherUser();
    WAIT_UNTIL(bob.isAtWarWith(cAlice));
  }

  SECTION("War is declared, then Bob logs in") {
    s.wars().declare(b1, b2);
    WAIT_UNTIL(s.wars().isAtWar(b1, b2));
    TestClient bob = TestClient::WithUsername("Bob");

    // Then Bob is at war with Alice;
    CHECK(s.wars().isAtWar("Alice", "Bob"));

    // And Bob knows that he's at war with Alice
    WAIT_UNTIL(bob.otherUsers().size() == 1);
    const auto &cAlice = bob.getFirstOtherUser();
    WAIT_UNTIL(bob.isAtWarWith(cAlice));
  }
}

TEST_CASE("Players can declare war on cities", "[city][war]") {
  // Given a running server;
  TestServer s;

  // And a user, Alice;
  TestClient alice = TestClient::WithUsername("Alice");

  // And a city named Athens;
  s.cities().createCity("Athens", {}, {});

  // When Alice declares war on Athens
  s.waitForUsers(1);
  alice.sendMessage(CL_DECLARE_WAR_ON_CITY, "Athens");

  // Then they are at war
  Belligerent b1("Alice", Belligerent::PLAYER), b2("Athens", Belligerent::CITY);
  WAIT_UNTIL(s.wars().isAtWar(b1, b2));
}

TEST_CASE("Wars involving cities are persistent", "[persistence][city][war]") {
  Belligerent b1("Alice", Belligerent::PLAYER), b2("Athens", Belligerent::CITY);

  {
    // Given a city named Athens;
    TestServer server1;
    server1.cities().createCity("Athens", {}, {});

    // And Alice and Athens are at war;
    server1.wars().declare(b1, b2);

    // And there is no server running
  }

  // When a server begins that keeps persistent data
  TestServer server2 = TestServer::KeepingOldData();

  // Then Alice and Athens are still at war
  CHECK(server2.wars().isAtWar(b1, b2));
}

TEST_CASE("The objects of an offline enemy in an enemy city can be attacked",
          "[city][war][permissions]") {
  GIVEN("a Chair object type") {
    TestServer s = TestServer::WithData("chair");

    AND_GIVEN("Bob is an offline member of Athens") {
      s.cities().createCity("Athens", {}, {});
      {
        auto bob = TestClient::WithUsernameAndData("Bob", "chair");
        s.waitForUsers(1);
        s.cities().addPlayerToCity(s.getFirstUser(), "Athens");
      }
      s.waitForUsers(0);

      AND_GIVEN("Bob owns a chair") {
        s.addObject("chair", {15, 15}, "Bob");

        AND_GIVEN("Alice is at war with Athens") {
          TestClient alice = TestClient::WithUsernameAndData("Alice", "chair");
          s.wars().declare("Alice", Belligerent("Athens", Belligerent::CITY));

          WHEN("Alice becomes aware of the rock") {
            WAIT_UNTIL(alice.objects().size() == 1);

            THEN("Alice can attack the rock") {
              const auto &rock = alice.getFirstObject();
              WAIT_UNTIL(rock.canBeAttackedByPlayer());
            }
          }
        }
      }
    }
  }
}

TEST_CASE("A player is alerted when he sues for peace", "[war]") {
  // Given Alice and Bob are at war
  auto s = TestServer{};
  s.wars().declare({"Alice", Belligerent::PLAYER},
                   {"Bob", Belligerent::PLAYER});

  // When Alice sues for peace
  auto c = TestClient::WithUsername("Alice");
  s.waitForUsers(1);
  c.sendMessage(CL_SUE_FOR_PEACE_WITH_PLAYER, "Bob");

  // Then Alice is alerted
  CHECK(c.waitForMessage(SV_YOU_PROPOSED_PEACE_TO_PLAYER));
}

TEST_CASE("The enemy is alerted when peace is proposed", "[war]") {
  // Given Alice and Bob are at war
  auto s = TestServer{};
  s.wars().declare({"Alice", Belligerent::PLAYER},
                   {"Bob", Belligerent::PLAYER});

  // And Alice is logged in
  auto c = TestClient::WithUsername("Alice");

  // When Bob logs in and sues for peace
  auto c2 = TestClient::WithUsername("Bob");
  s.waitForUsers(2);
  auto &bob = s.findUser("Bob");
  s->handleBufferedMessages(
      bob.socket(), Message{CL_SUE_FOR_PEACE_WITH_PLAYER, "Alice"}.compile());

  // Then Alice is alerted
  CHECK(c.waitForMessage(SV_PEACE_WAS_PROPOSED_TO_YOU_BY_PLAYER));
}

TEST_CASE("Users are alerted to peace proposals on login", "[war]") {
  // Given Alice and Bob are at war
  auto s = TestServer{};
  s.wars().declare({"Alice", Belligerent::PLAYER},
                   {"Bob", Belligerent::PLAYER});

  {
    // When Alice sues for peace
    auto c = TestClient::WithUsername("Alice");
    s.waitForUsers(1);
    c.sendMessage(CL_SUE_FOR_PEACE_WITH_PLAYER, "Bob");

    // And disconnects
  }

  SECTION("Alice logs in") {
    auto c = TestClient::WithUsername("Alice");

    // Then Alice is alerted
    CHECK(c.waitForMessage(SV_YOU_PROPOSED_PEACE_TO_PLAYER));
  }

  SECTION("Bob logs in") {
    auto c = TestClient::WithUsername("Bob");

    // Then Bob is alerted
    CHECK(c.waitForMessage(SV_PEACE_WAS_PROPOSED_TO_YOU_BY_PLAYER));
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "War-declaration debuffs",
                 "[buffs][war][city]") {
  GIVEN("two debuffs for those who declare war, plus one unused") {
    useData(R"(
      <buff id="frownedUpon" duration="60" givenToDeclarersOfWar="1" />
      <buff id="regretful" duration="60" givenToDeclarersOfWar="1" />
      <buff id="happy" duration="60" />
    )");

    WHEN("Alice declares war on Bob") {
      cAlice->sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

      THEN("Alice has two debuffs") {
        REPEAT_FOR_MS(100);
        CHECK(uAlice->debuffs().size() == 2);
      }
    }

    SECTION("When a city declares war") {
      AND_GIVEN("Alice is in a city") {
        server->createCityWithUserAsKing("Athens", *uAlice);

        WHEN("Alice declares a city war on Bob") {
          cAlice->sendMessage(CL_DECLARE_WAR_ON_PLAYER_AS_CITY, "Bob");

          THEN("Alice has two debuffs") {
            WAIT_UNTIL(uAlice->debuffs().size() == 2);
          }
        }

        SECTION("All citizens get debuffed") {
          AND_GIVEN("Bob is in the city") {
            server->cities().addPlayerToCity(*uBob, "Athens");

            WHEN("Alice declares a city war on Charlie") {
              cAlice->sendMessage(CL_DECLARE_WAR_ON_PLAYER_AS_CITY, "Charlie");

              THEN("Bob has two debuffs") {
                WAIT_UNTIL(uBob->debuffs().size() == 2);
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE("War-declaration debuffs for offline citizens",
          "[buffs][war][city][persistence]") {
  GIVEN("a one-second war-declaration debuff") {
    const auto data = R"(
      <buff id="frownedUpon" duration="1" givenToDeclarersOfWar="1" />
    )";
    auto server = TestServer::WithDataString(data);

    AND_GIVEN("Alice is king of Athens") {
      auto cAlice = TestClient::WithUsernameAndDataString("Alice", data);
      server.waitForUsers(1);
      const auto &uAlice = server.findUser("Alice");
      server.createCityWithUserAsKing("Athens", uAlice);

      AND_GIVEN("Bob is also in Athens, and Bob is offline") {
        {
          auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
          server.waitForUsers(2);
          const auto &uBob = server.findUser("Bob");
          server.cities().addPlayerToCity(uBob, "Athens");
        }

        WHEN("Alice declares a city war on Charlie") {
          cAlice.sendMessage(CL_DECLARE_WAR_ON_PLAYER_AS_CITY, "Charlie");

          AND_WHEN("Bob comes online") {
            auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
            server.waitForUsers(2);
            const auto &uBob = server.findUser("Bob");

            THEN("Bob has a debuff") { WAIT_UNTIL(uBob.debuffs().size() == 1); }
          }

          SECTION("Debuff timing") {
            AND_WHEN(
                "at least 1s elapses (enough for the debuff to disappear)") {
              REPEAT_FOR_MS(1100);

              AND_WHEN("Bob comes online") {
                auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
                server.waitForUsers(2);
                const auto &uBob = server.findUser("Bob");

                THEN("Bob has no debuffs") {
                  REPEAT_FOR_MS(100);
                  CHECK(uBob.debuffs().empty());
                }
              }
            }
          }
        }

        SECTION("City did not declare any war") {
          AND_WHEN("Bob comes online") {
            auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
            server.waitForUsers(2);
            const auto &uBob = server.findUser("Bob");

            THEN("Bob has no debuffs") {
              REPEAT_FOR_MS(100);
              CHECK(uBob.debuffs().empty());
            }
          }
        }
      }

      SECTION("A different city declares war") {
        AND_GIVEN("Bob is king of Sparta, and is offline") {
          {
            auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
            server.waitForUsers(2);
            const auto &uBob = server.findUser("Bob");
            server.createCityWithUserAsKing("Sparta", uBob);
          }

          WHEN("Alice declares a city war on Charlie") {
            cAlice.sendMessage(CL_DECLARE_WAR_ON_PLAYER_AS_CITY, "Charlie");

            AND_WHEN("Bob comes online") {
              auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
              server.waitForUsers(2);
              const auto &uBob = server.findUser("Bob");

              THEN("Bob has no debuffs") {
                REPEAT_FOR_MS(100);
                CHECK(uBob.debuffs().empty());
              }
            }
          }
        }
      }
    }
  }
}
