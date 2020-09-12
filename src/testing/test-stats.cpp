#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Leveling up restores health and energy") {
  auto s = TestServer{};
  auto c = TestClient{};

  // Given a damaged user
  s.waitForUsers(1);
  auto &user = s.getFirstUser();
  user.reduceHealth(1);
  user.reduceEnergy(1);
  CHECK(user.health() < user.stats().maxHealth);
  CHECK(user.energy() < user.stats().maxEnergy);

  // When the user levels up
  user.addXP(User::XP_PER_LEVEL[1]);

  // Then the user's health and energy are full
  WAIT_UNTIL(user.health() == user.stats().maxHealth);
  WAIT_UNTIL(user.energy() == user.stats().maxEnergy);
}

TEST_CASE("Client has correct XP on level up") {
  GIVEN("A player") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("he gets enough XP to level up") {
      auto xpToLevel = User::XP_PER_LEVEL[user.level()];
      user.addXP(xpToLevel);

      THEN("he knows he has 0 XP") {
        REPEAT_FOR_MS(100);
        CHECK(c->xp() == 0);
      }
    }
  }
}

TEST_CASE("Follower-limit stat") {
  GIVEN("a user") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    THEN("he knows his follower limit is 1") {
      WAIT_UNTIL(c.stats().followerLimit == 1);

      AND_WHEN("the baseline is changed to 2") {
        auto oldStats = User::OBJECT_TYPE.baseStats();
        auto highFollowerLimitStats = oldStats;
        highFollowerLimitStats.followerLimit = 2;
        User::OBJECT_TYPE.baseStats(highFollowerLimitStats);
        auto &user = s.getFirstUser();
        user.updateStats();

        THEN("he knows his limit is 2") {
          WAIT_UNTIL(c.stats().followerLimit == 2);
        }

        User::OBJECT_TYPE.baseStats(oldStats);
      }
    }
  }

  GIVEN("gear that gives +1 follower count") {
    auto data = R"(
      <item id="bait" gearSlot="7" >
        <stats followerLimit="1" />
      </item>
    )";

    auto s = TestServer::WithDataString(data);

    THEN("it has that stat") {
      const auto &bait = s.getFirstItem();
      CHECK(bait.stats().followerLimit == 1);

      AND_GIVEN("a user") {
        auto c = TestClient::WithDataString(data);
        s.waitForUsers(1);
        auto &user = s.getFirstUser();

        WHEN("he has the gear equipped") {
          user.giveItem(&bait);
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Inventory(), 0, Serial::Gear(), 7));

          THEN("his follower count is 2") {
            WAIT_UNTIL(user.stats().followerLimit == 2);
          }
        }
      }
    }
  }

  GIVEN("gear with very negative follower count") {
    auto data = R"(
      <item id="megaphone" gearSlot="7" >
        <stats followerLimit="-1000" />
      </item>
    )";

    auto s = TestServer::WithDataString(data);

    AND_GIVEN("a user") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      WHEN("he has the gear equipped") {
        const auto &megaphone = s.getFirstItem();
        user.giveItem(&megaphone);
        c.sendMessage(CL_SWAP_ITEMS,
                      makeArgs(Serial::Inventory(), 0, Serial::Gear(), 7));

        THEN("his follower count is 0") {
          WAIT_UNTIL(user.stats().followerLimit == 0);
        }
      }
    }
  }
}

TEST_CASE("Speed stat") {
  GIVEN("a server and client") {
    auto s = TestServer{};
    auto c = TestClient{};

    WHEN("the client receives his stats") {
      CHECK(c.waitForMessage(SV_YOUR_STATS));

      THEN("his speed is valid") { CHECK(c.stats().speed > 0); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Armour stat") {
  GIVEN("an NPC type that deals 50 damage") {
    useData(R"(
      <npcType id="fox" attack="50" />
      <npcType id="magicFox"> <spell id="physical50"/> </npcType>

      <spell id="physical50" school="physical" range="30" cooldown="1">
        <targets enemy="1" />
        <function name="doDirectDamage" i1="50" />
      </spell>
    )");

    AND_GIVEN("players have 500 armour [50% reduction]") {
      auto oldStats = User::OBJECT_TYPE.baseStats();
      auto with500AC = oldStats;
      with500AC.armor = 500;
      User::OBJECT_TYPE.baseStats(with500AC);
      user->updateStats();
      auto healthBefore = user->health();

      WHEN("an NPC hits the player for 50 physical") {
        server->addNPC("fox", {15, 15});
        WAIT_UNTIL(user->health() < healthBefore);

        THEN("the player loses around 25 health") {
          CHECK_ROUGHLY_EQUAL(1. * user->health(), 1. * healthBefore - 25, .1);
        }
      }

      WHEN("an NPC hits the player with a 50-physical spell") {
        server->addNPC("magicFox", {15, 15});
        WAIT_UNTIL(user->health() < healthBefore);

        THEN("the player loses around 25 health") {
          CHECK_ROUGHLY_EQUAL(1. * user->health(), 1. * healthBefore - 25, .1);
        }
      }

      User::OBJECT_TYPE.baseStats(oldStats);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Hit chance") {
  // Assumption: base miss chance of 10%

  GIVEN("NPCs have 5% hit chance") {
    auto oldStats = NPCType::BASE_STATS;
    NPCType::BASE_STATS.hit = 500;
    useData(R"(
      <npcType id="dog" attack="1" />
    )");
    NPCType::BASE_STATS = oldStats;

    AND_GIVEN("an NPC") {
      const auto &npc = server->addNPC("dog", {15, 15});

      WHEN("100000 hits are generated for it") {
        auto numMisses = 0;
        for (auto i = 0; i != 100000; ++i) {
          auto result = npc.generateHitAgainst(npc, DAMAGE, {}, 0);
          if (result == MISS) ++numMisses;
        }

        THEN("around 5% of them are misses") {
          CHECK_ROUGHLY_EQUAL(numMisses, 5000, 0.1)
        }
      }
    }
  }

  GIVEN("users have 5% hit chance") {
    useData("");
    auto oldStats = User::OBJECT_TYPE.baseStats();
    auto with500Hit = oldStats;
    with500Hit.hit = 500;
    User::OBJECT_TYPE.baseStats(with500Hit);
    user->updateStats();

    WHEN("100000 hits are generated for a user") {
      auto numMisses = 0;
      for (auto i = 0; i != 100000; ++i) {
        auto result = user->generateHitAgainst(*user, DAMAGE, {}, 0);
        if (result == MISS) ++numMisses;
      }

      THEN("around 5% of them are misses") {
        CHECK_ROUGHLY_EQUAL(numMisses, 5000, 0.1)
      }
    }

    User::OBJECT_TYPE.baseStats(oldStats);
  }
}

TEST_CASE_METHOD(ServerAndClient, "Crit chance") {
  GIVEN("users have 50% crit chance") {
    auto oldStats = User::OBJECT_TYPE.baseStats();
    auto with5000Crit = oldStats;
    with5000Crit.crit = 5000;
    User::OBJECT_TYPE.baseStats(with5000Crit);
    user->updateStats();

    WHEN("10000 hits are generated for a user") {
      auto numCrits = 0;
      for (auto i = 0; i != 10000; ++i) {
        auto result = user->generateHitAgainst(*user, DAMAGE, {}, 0);
        if (result == CRIT) ++numCrits;
      }

      THEN("around 50% of them are crits") {
        CHECK_ROUGHLY_EQUAL(numCrits, 5000, 0.1)
      }
    }

    User::OBJECT_TYPE.baseStats(oldStats);
  }
}

TEST_CASE_METHOD(ServerAndClient, "Crit resistance") {
  GIVEN("users have 50% crit chance and 25% crit resistance") {
    auto oldStats = User::OBJECT_TYPE.baseStats();
    auto with2500CritResist = oldStats;
    with2500CritResist.crit = 5000;
    with2500CritResist.critResist = 2500;
    User::OBJECT_TYPE.baseStats(with2500CritResist);
    user->updateStats();

    WHEN("10000 hits are generated for a user") {
      auto numCrits = 0;
      for (auto i = 0; i != 10000; ++i) {
        auto result = user->generateHitAgainst(*user, DAMAGE, {}, 0);
        if (result == CRIT) ++numCrits;
      }

      THEN("around 25% of them are crits") {
        CHECK_ROUGHLY_EQUAL(numCrits, 2500, 0.1)
      }
    }

    User::OBJECT_TYPE.baseStats(oldStats);
  }
}

TEST_CASE_METHOD(ServerAndClient, "Dodge chance") {
  GIVEN("users have 50% dodge chance") {
    auto oldStats = User::OBJECT_TYPE.baseStats();
    auto with5000Dodge = oldStats;
    with5000Dodge.dodge = 5000;
    User::OBJECT_TYPE.baseStats(with5000Dodge);
    user->updateStats();

    WHEN("10000 hits are generated against a user") {
      auto numDodges = 0;
      for (auto i = 0; i != 10000; ++i) {
        auto result = user->generateHitAgainst(*user, DAMAGE, {}, 0);
        if (result == DODGE) ++numDodges;
      }

      THEN("around 50% of them are dodges") {
        CHECK_ROUGHLY_EQUAL(numDodges, 5000, 0.1)
      }
    }

    User::OBJECT_TYPE.baseStats(oldStats);
  }
}

// Block
// Gather bonus
