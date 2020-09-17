#include "TemporaryUserStats.h"
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
        CHANGE_BASE_USER_STATS.followerLimit(2);
        auto &user = s.getFirstUser();
        user.updateStats();

        THEN("he knows his limit is 2") {
          WAIT_UNTIL(c.stats().followerLimit == 2);
        }
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

TEST_CASE_METHOD(ServerAndClient, "Speed stat") {
  THEN("the client receives a valid speed") {
    WAIT_UNTIL(client.stats().speed > 0);
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Damage reduction from armour") {
  GIVEN("an NPC type that deals 50 damage") {
    auto oldNPCStats = NPCType::BASE_STATS;
    NPCType::BASE_STATS.crit = 0;
    NPCType::BASE_STATS.hit = 20000;

    useData(R"(
      <npcType id="fox" attack="50" />
      <npcType id="magicFox"> <spell id="physical50"/> </npcType>

      <spell id="physical50" school="physical" range="30" cooldown="1">
        <targets enemy="1" />
        <function name="doDirectDamage" i1="50" />
      </spell>
    )");

    NPCType::BASE_STATS = oldNPCStats;

    AND_GIVEN("players have 500 armour [50% reduction]") {
      CHANGE_BASE_USER_STATS.armour(500);
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
    }
  }
}

TEST_CASE("Armour is clamped to 0-1000") {
  CHECK(ArmourClass{-100}.applyTo(100.0) == 100.0);
  CHECK(ArmourClass{2000}.applyTo(100.0) == 0);
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Resistances are affected by level differences") {
  GIVEN("a lvl-21 NPC type with high health, and a 1000-damage spell") {
    useData(R"(
      <npcType id="rockGiant" level="21" maxHealth="1000000" />

      <spell id="bomb" school="fire" range="30" cooldown="1">
        <targets enemy="1" />
        <function name="doDirectDamage" i1="1000" />
      </spell>
    )");

    const auto lvlDiff = 20;
    const auto reductionFromLvlDiff = 1.0 - (0.01 * lvlDiff * 3);
    const auto expectedDamage = 1000.0 * reductionFromLvlDiff;

    SECTION("Spell damage") {
      AND_GIVEN("an NPC out of combat range of the user") {
        const auto &rockGiant = server->addNPC("rockGiant", {100, 100});

        AND_GIVEN("players have 200% hit and 0% crit") {
          CHANGE_BASE_USER_STATS.hit(20000).crit(0);
          user->updateStats();

          AND_GIVEN("the player knows the spell") {
            user->getClass().teachSpell("bomb");

            WHEN("he casts it at the NPC") {
              client->sendMessage(CL_TARGET_ENTITY,
                                  makeArgs(rockGiant.serial()));
              client->sendMessage(CL_CAST, "bomb");
              const auto oldHealth = rockGiant.health();
              WAIT_UNTIL(rockGiant.health() < oldHealth);

              THEN("the damage is reduced by ~60%") {
                const auto healthLost = 1.0 * oldHealth - rockGiant.health();
                CHECK_ROUGHLY_EQUAL(expectedDamage, healthLost, 0.3);
              }
            }
          }
        }
      }
    }

    SECTION("Combat damage") {
      AND_GIVEN("players always hit for 1000 damage") {
        CHANGE_BASE_USER_STATS.hit(20000).crit(0).weaponDamage(1000);
        user->updateStats();

        AND_GIVEN("an NPC close to the user") {
          const auto &rockGiant = server->addNPC("rockGiant", {15, 15});
          const auto oldHealth = rockGiant.health();

          WHEN("the user hits the NPC") {
            client->sendMessage(CL_TARGET_ENTITY, makeArgs(rockGiant.serial()));
            WAIT_UNTIL_TIMEOUT(rockGiant.health() < oldHealth, 5000);

            THEN("the damage is reduced by 60%") {
              const auto healthLost = 1.0 * oldHealth - rockGiant.health();
              CHECK_ROUGHLY_EQUAL(expectedDamage, healthLost, 0.3);
            }
          }
        }
      }
    }
  }

  CHECK(ArmourClass{100}.modifyByLevelDiff(2, 1) == ArmourClass{70});
  CHECK(ArmourClass{500}.modifyByLevelDiff(2, 1) == ArmourClass{470});
  CHECK(ArmourClass{500}.modifyByLevelDiff(3, 2) == ArmourClass{470});
  CHECK(ArmourClass{500}.modifyByLevelDiff(3, 1) == ArmourClass{440});
  CHECK(ArmourClass{500}.modifyByLevelDiff(4, 1) == ArmourClass{410});
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
    CHANGE_BASE_USER_STATS.hit(500);
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
  }
}

TEST_CASE_METHOD(ServerAndClient, "Crit chance") {
  GIVEN("users have 50% crit chance") {
    CHANGE_BASE_USER_STATS.crit(5000);
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
  }
}

TEST_CASE_METHOD(ServerAndClient, "Crit resistance") {
  GIVEN("users have 50% crit chance and 25% crit resistance") {
    CHANGE_BASE_USER_STATS.crit(5000).critResist(2500);
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
  }
}

TEST_CASE_METHOD(ServerAndClient, "Dodge chance") {
  GIVEN("users have 50% dodge chance") {
    CHANGE_BASE_USER_STATS.dodge(5000);
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
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Block chance") {
  useData(R"(
    <item id="shield" gearSlot="7">
      <tag name="shield" />
    </item>
    )");

  GIVEN("users have 50% block chance") {
    CHANGE_BASE_USER_STATS.block(5000);

    AND_GIVEN("the user has a shield") {
      auto &shield = server->getFirstItem();
      user->giveItem(&shield);
      client->sendMessage(
          CL_SWAP_ITEMS,
          makeArgs(Serial::Inventory(), 0, Serial::Gear(), Item::OFFHAND_SLOT));
      WAIT_UNTIL(user->gear(Item::OFFHAND_SLOT).first.hasItem());

      WHEN("10000 hits are generated against him") {
        auto numBlocks = 0;
        for (auto i = 0; i != 10000; ++i) {
          auto result = user->generateHitAgainst(*user, DAMAGE, {}, 0);
          if (result == BLOCK) ++numBlocks;
        }

        THEN("around 50% of them are blocks") {
          CHECK_ROUGHLY_EQUAL(numBlocks, 5000, 0.1)
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Gather-bonus stat") {
  GIVEN("an object with a large yield, one at a time") {
    useData(R"(
      <item id="tissue" stackSize="10000000" />
      <objectType id="tissueBox">
        <yield id="tissue" initialMean="1000000" />
      </objectType>
    )");
    const auto &tissueBox = server->addObject("tissueBox", {15, 15});

    AND_GIVEN("users have a 50% gather bonus") {
      CHANGE_BASE_USER_STATS.gatherBonus(50);
      user->updateStats();

      WHEN("the user gathers from it 1000 times") {
        for (auto i = 0; i != 1000; ++i)
          server->gatherObject(tissueBox.serial(), *user);

        THEN("he has around 1500 items") {
          auto numTissuesGathered = int(user->inventory(0).second);
          CHECK_ROUGHLY_EQUAL(numTissuesGathered, 1500, 0.1);
        }
      }
    }
  }
}

TEST_CASE("Basis-point display") {
  CHECK(BasisPoints{0}.display() == "0.00%"s);
  CHECK(BasisPoints{1}.display() == "0.01%"s);
  CHECK(BasisPoints{50}.display() == "0.50%"s);
  CHECK(BasisPoints{100}.display() == "1.00%"s);
  CHECK(BasisPoints{100}.displayShort() == "1%"s);
  CHECK(BasisPoints{200}.display() == "2.00%"s);
  CHECK(BasisPoints{200}.displayShort() == "2%"s);
}

TEST_CASE("Basis-point stats are read as percentages") {
  GIVEN("an item specified to grant \"1 crit\" and \"2 dodge\"") {
    auto data = R"(
      <item id="critSword" >
        <stats crit="1" dodge="2" />
      </item>
    )";

    WHEN("a server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("the item has 100 crit and 200 dodge") {
        auto &critSword = s.getFirstItem();
        CHECK(critSword.stats().crit == BasisPoints{100});
        CHECK(critSword.stats().dodge == BasisPoints{200});
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Health regen") {
  GIVEN("a user is missing 10 health") {
    user->reduceHealth(10);
    auto oldHealth = user->health();

    AND_GIVEN("users regenerate 1 health per second") {
      CHANGE_BASE_USER_STATS.hps(100);
      user->updateStats();

      WHEN("a little over 1s passes") {
        REPEAT_FOR_MS(1010);

        THEN("his health has increased by 1") {
          CHECK(user->health() == oldHealth + 1);
        }
      }
    }
    AND_GIVEN("users regenerate 2 health per second") {
      CHANGE_BASE_USER_STATS.hps(200);
      user->updateStats();
      CHECK(user->stats().hps == Regen{200});

      WHEN("a little over 1s passes") {
        REPEAT_FOR_MS(1010);

        THEN("his health has increased by 2") {
          CHECK(user->health() == oldHealth + 2);
        }
      }
    }

    AND_GIVEN("users regenerate 0.5 health per second") {
      CHANGE_BASE_USER_STATS.hps(50);
      user->updateStats();

      WHEN("a little over 2s passes") {
        REPEAT_FOR_MS(2010);

        THEN("his health has increased by 1") {
          CHECK(user->health() == oldHealth + 1);
        }
      }
    }
  }
}

TEST_CASE("Bonus-damage stat") {
  struct TestValues {
    BasisPoints bonusPhysicalDamage;
    double expectedDamage;
  };
  auto tests =
      std::vector<TestValues>{{10000, 200.0}, {20000, 300.0}, {10, 100.1}};

  for (auto testValues : tests) {
    GIVEN("NPCs have +" + testValues.bonusPhysicalDamage.display() +
          " bonus physical damage") {
      auto oldStats = NPCType::BASE_STATS;
      NPCType::BASE_STATS.physicalDamage = testValues.bonusPhysicalDamage;

      AND_GIVEN("elephants have 100 base attack") {
        auto s = TestServer::WithDataString(R"(
          <npcType id="elephant" attack="100" />
        )");

        AND_GIVEN("an elephant") {
          auto &elephant = s.addNPC("elephant", {10, 10});

          THEN("the elephant does ~" + toString(testValues.expectedDamage) +
               " damage") {
            CHECK(elephant.combatDamage() == testValues.expectedDamage);
          }
        }
      }

      NPCType::BASE_STATS = oldStats;
    }
  }

  GIVEN("NPCs have +100% bonus magic damage") {
    auto oldStats = NPCType::BASE_STATS;
    NPCType::BASE_STATS.magicDamage = 10000;

    AND_GIVEN("a whale with 100 base water attack") {
      auto s = TestServer::WithDataString(R"(
        <npcType id="whale" attack="100" school="water" />
      )");
      auto &whale = s.addNPC("whale", {10, 10});

      THEN("it does ~200 damage") { CHECK(whale.combatDamage() == 200.0); }
    }

    NPCType::BASE_STATS = oldStats;
  }

  GIVEN("NPCs have +100% magic and +200% physical damage") {
    auto oldStats = NPCType::BASE_STATS;
    NPCType::BASE_STATS.magicDamage = 10000;
    NPCType::BASE_STATS.physicalDamage = 20000;

    AND_GIVEN("a whale with 100 base water attack") {
      auto s = TestServer::WithDataString(R"(
        <npcType id="whale" attack="100" school="water" />
      )");
      auto &whale = s.addNPC("whale", {10, 10});

      THEN("it does ~200 damage") { CHECK(whale.combatDamage() == 200.0); }
    }

    NPCType::BASE_STATS = oldStats;
  }
}
