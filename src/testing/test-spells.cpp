#include "TemporaryUserStats.h"
#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Non-talent spells", "[spells]") {
  GIVEN("a spell") {
    auto data = R"(
      <spell id="fireball" >
        <targets enemy=1 />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    CHECK_FALSE(user.getClass().knowsSpell("fireball"));

    WHEN("a player learns it") {
      user.getClass().teachSpell("fireball");

      THEN("he knows it") {
        CHECK(user.getClass().knowsSpell("fireball"));
        WAIT_UNTIL(c.knowsSpell("fireball"));

        AND_THEN("he doesn't know some other spell") {
          CHECK_FALSE(user.getClass().knowsSpell("iceball"));
          CHECK_FALSE(c.knowsSpell("iceball"));
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Spells that can target only a specific NPC", "[spells]") {
  SECTION("Direct-target spell") {
    GIVEN("a shiver-timbers spell that can only be cast on pirates") {
      useData(R"(
        <spell id="shiverTimbers" >
          <targets specificNPC="pirate" enemy="1" />
          <function name="doDirectDamage" i1="1" />
        </spell>
        <npcType id="pirate" maxHealth="1000" attack="1" />
        <npcType id="vampire" maxHealth="1000" attack="1" />
      )");
      const auto &shiverTimbers = server->getFirstSpell();

      AND_GIVEN("user spells never miss") {
        CHANGE_BASE_USER_STATS.hit(10000);
        user->updateStats();

        AND_GIVEN("the user knows the spell") {
          user->getClass().teachSpell("shiverTimbers");

          AND_GIVEN("a vampire") {
            auto &vampire = server->addNPC("vampire", {20.0, 20.0});

            WHEN("the user tries to shiver the vampire's timbers") {
              user->target(&vampire);
              const auto result = user->castSpell(shiverTimbers);

              THEN("the spellcast fails") {
                CHECK(result == CombatResult::FAIL);
              }
            }
          }

          AND_GIVEN("a pirate") {
            auto &pirate = server->addNPC("pirate", {20.0, 20.0});

            WHEN("the user tries to shiver the pirate's timbers") {
              user->target(&pirate);
              const auto result = user->castSpell(shiverTimbers);

              THEN("the spellcast succeeds") {
                CHECK(result == CombatResult::HIT);
              }
            }
          }
        }
      }
    }
  }

  SECTION("AoE spell") {
    GIVEN("a shiver-timbers AoE spell that can only be cast on pirates") {
      useData(R"(
        <spell id="shiverTimbers" radius="100" >
          <targets specificNPC="pirate" enemy="1" />
          <function name="doDirectDamage" i1="5" />
        </spell>
        <npcType id="pirate" maxHealth="1000" />
        <npcType id="vampire" maxHealth="1000" />
      )");
      const auto &shiverTimbers = server->getFirstSpell();

      AND_GIVEN("user spells never miss") {
        CHANGE_BASE_USER_STATS.hit(10000);
        user->updateStats();

        AND_GIVEN("the user knows the spell") {
          user->getClass().teachSpell("shiverTimbers");

          AND_GIVEN("a vampire and a pirate") {
            auto &vampire = server->addNPC("vampire", {20.0, 30.0});
            auto &pirate = server->addNPC("pirate", {30.0, 20.0});

            WHEN("the user tries to shiver everyone's timbers") {
              const auto result = user->castSpell(shiverTimbers);
              REPEAT_FOR_MS(100);

              THEN("the pirate is affected") {
                CHECK(pirate.isMissingHealth());

                AND_THEN("the vampire is not") {
                  CHECK(!vampire.isMissingHealth());
                }
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Non-talent spells are persistent", "[spells][persistence]") {
  // Given a spell
  auto data = R"(
    <spell id="fireball" >
      <targets enemy=1 />
    </spell>
  )";
  {
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    // When Alice learns it
    user.getClass().teachSpell("fireball");

    // And the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);
    auto c = TestClient::WithUsernameAndDataString("alice", data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    // Then she still knows it
    WAIT_UNTIL(user.getClass().knowsSpell("fireball"));

    // And she knows it
    WAIT_UNTIL(c.knowsSpell("fireball"));
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "Spell cooldowns", "[spells]") {
  GIVEN(
      "Alice and Bob know a range of self-damaging spells, and have extremely "
      "high hit chance") {
    useData(R"(
      <spell id="hurtSelf1s" cooldown="1" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
      <spell id="hurtSelfAlso1s" cooldown="1" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
      <spell id="hurtSelf2s" cooldown="2" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
      <spell id="hurtSelf" >
        <targets self=1 />
        <function name="doDirectDamage" i1="5" />
      </spell>
    )");

    auto oldStats = User::OBJECT_TYPE.baseStats();
    auto highHitChance = oldStats;
    highHitChance.hit = 100;
    User::OBJECT_TYPE.baseStats(highHitChance);

    uAlice->getClass().teachSpell("hurtSelf");
    uAlice->getClass().teachSpell("hurtSelf1s");
    uAlice->getClass().teachSpell("hurtSelfAlso1s");
    uAlice->getClass().teachSpell("hurtSelf2s");

    WHEN("Alice casts the spell with cooldown 1s") {
      cAlice->sendMessage(CL_CAST_SPELL, "hurtSelf1s");
      REPEAT_FOR_MS(100);
      auto healthAfterFirstCast = uAlice->health();

      AND_WHEN("she tries casting it again") {
        cAlice->sendMessage(CL_CAST_SPELL, "hurtSelf1s");

        THEN("she hasn't lost any health") {
          REPEAT_FOR_MS(100);
          CHECK(uAlice->health() >= healthAfterFirstCast);
        }
      }

      AND_WHEN("she tries a different spell with cooldown 1s") {
        cAlice->sendMessage(CL_CAST_SPELL, "hurtSelfAlso1s");

        THEN("she has lost health") {
          REPEAT_FOR_MS(100);
          CHECK(uAlice->health() < healthAfterFirstCast);
        }
      }

      AND_WHEN("1s elapses") {
        REPEAT_FOR_MS(1000);

        AND_WHEN("she tries casting it again") {
          cAlice->sendMessage(CL_CAST_SPELL, "hurtSelf1s");

          THEN("she has lost health") {
            REPEAT_FOR_MS(100);
            CHECK(uAlice->health() < healthAfterFirstCast);
          }
        }
      }

      AND_WHEN("she tries casting the spell with no cooldown") {
        cAlice->sendMessage(CL_CAST_SPELL, "hurtSelf");

        THEN("she has lost health") {
          REPEAT_FOR_MS(100);
          CHECK(uAlice->health() < healthAfterFirstCast);
        }
      }

      AND_WHEN("Bob tries casting it") {
        uBob->getClass().teachSpell("hurtSelf1s");

        const auto healthBefore = uBob->health();
        cBob->sendMessage(CL_CAST_SPELL, "hurtSelf1s");

        THEN("he has lost health") {
          REPEAT_FOR_MS(100);
          CHECK(uBob->health() < healthBefore);
        }
      }
    }

    WHEN("Alice casts the spell with cooldown 2s") {
      cAlice->sendMessage(CL_CAST_SPELL, "hurtSelf2s");
      REPEAT_FOR_MS(100);
      auto healthAfterFirstCast = uAlice->health();

      AND_WHEN("1s elapses") {
        REPEAT_FOR_MS(1000);

        AND_WHEN("she tries casting it again") {
          cAlice->sendMessage(CL_CAST_SPELL, "hurtSelf2s");

          THEN("she hasn't lost any health") {
            REPEAT_FOR_MS(100);
            CHECK(uAlice->health() >= healthAfterFirstCast);
          }
        }
      }
    }
    User::OBJECT_TYPE.baseStats(oldStats);
  }

  SECTION("Persistence") {
    // Given Alice knows a spell with a 2s cooldown"
    auto data = R"(
      <spell id="nop" cooldown="2" >
        <targets self=1 />
        <function name="doDirectDamage" i1="0" />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);

    {
      auto c = TestClient::WithUsernameAndDataString("Alice", data);
      s.waitForUsers(1);
      auto &alice = s.getFirstUser();
      alice.getClass().teachSpell("nop");

      // When she casts it
      c.sendMessage(CL_CAST_SPELL, "nop");
      WAIT_UNTIL(alice.isSpellCoolingDown("nop"));

      // And when she logs out and back in
    }
    {
      auto c = TestClient::WithUsernameAndDataString("Alice", data);
      s.waitForUsers(1);
      auto &alice = s.getFirstUser();

      // Then the spell is still cooling down
      CHECK(alice.isSpellCoolingDown("nop"));

      // And when she logs out and back in after 2s
    }
    {
      REPEAT_FOR_MS(2000);
      auto c = TestClient::WithUsernameAndDataString("Alice", data);
      s.waitForUsers(1);
      auto &alice = s.getFirstUser();

      // Then the spell has finished cooling down
      CHECK_FALSE(alice.isSpellCoolingDown("nop"));
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "NPC spells", "[spells][ai]") {
  GIVEN(
      "A wizard with no combat damage, and a fireball spell with a 2s "
      "cooldown") {
    useData(R"(
      <spell id="fireball" range=30 cooldown=2 >
        <targets enemy=1 />
        <function name="doDirectDamage" i1=5 />
      </spell>
      <npcType id="wizard">
        <spell id="fireball" />
      </npcType>
    )");

    server->addNPC("wizard", {10, 15});

    WHEN("a player is nearby") {
      THEN("he gets damaged") {
        WAIT_UNTIL(user->isMissingHealth());

        SECTION("Spell cooldowns apply correctly") {
          AND_WHEN("1 more second elapses") {
            const auto oldHealth = user->health();
            const auto oldLocation = user->location();
            REPEAT_FOR_MS(1000);

            THEN("his health is unchanged") {
              // Proxies for the user not dying.  Before implementation, the
              // user died and respawned many times. If the effects on dying are
              // changed, perhaps this can be improved or simplified.
              const auto expectedHealthAfter1s =
                  oldHealth + User::OBJECT_TYPE.baseStats().hps;
              CHECK(user->health() == expectedHealthAfter1s);
              CHECK(user->location() == oldLocation);
            }
          }
        }
      }
    }
  }

  SECTION("Other NPCs don't get the spell") {
    GIVEN("sorcerers can self-buff, and apprentices can't") {
      useData(R"(
        <spell id="buffMagic" >
            <targets self=1 />
            <function name="buff" s1="magic" />
        </spell>
        <buff id="magic" />
        <npcType id="sorcerer">
          <spell id="buffMagic" />
        </npcType>
        <npcType id="apprentice" />
      )");

      WHEN("an apprentice is created") {
        const auto &apprentice = server->addNPC("apprentice", {10, 15});

        THEN("he has no buffs") {
          REPEAT_FOR_MS(100);
          CHECK(apprentice.buffs().empty());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "NPCs cast AoE spells even with no target selected",
                 "[spells][ai]") {
  GIVEN("an NPC with an AoE spell that targets enemies") {
    useData(R"(
      <spell id="explosion" radius="100" >
          <targets enemy="1" cooldown="1" />
          <function name="doDirectDamage" i1="5" />
      </spell>
      <npcType id="pyromaniac" maxHealth="1000" >
        <spell id="explosion" />
      </npcType>
    )");
    const auto &pyromaniac = server->addNPC("pyromaniac", {20, 20});

    AND_GIVEN("users can't attack") {
      CHANGE_BASE_USER_STATS.weaponDamage(0);
      user->updateStats();

      WHEN("enough time passes for the spell to be cast") {
        REPEAT_FOR_MS(100);

        THEN("a nearby user has taken damage") {
          CHECK(user->isMissingHealth());

          AND_THEN("The NPC doesn't target itself as an \"enemy\"") {
            CHECK(!pyromaniac.isMissingHealth());
          }
        }
      }
    }
  }
}

TEST_CASE("Kills with spells give XP", "[spells][leveling]") {
  GIVEN("a high-damage spell and low-health NPC") {
    auto data = R"(
      <spell id="nuke" range=30 >
        <targets enemy=1 />
        <function name="doDirectDamage" i1=99999 />
      </spell>
      <npcType id="critter" maxHealth=1 />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("critter", {10, 15});

    WHEN("a user knows the spell") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.getClass().teachSpell("nuke");

      AND_WHEN("he kills the NPC with the spell") {
        const auto &critter = s.getFirstNPC();
        c.sendMessage(CL_SELECT_ENTITY, makeArgs(critter.serial()));
        c.sendMessage(CL_CAST_SPELL, "nuke");
        WAIT_UNTIL(critter.isDead());

        THEN("the user has some XP") { WAIT_UNTIL(user.xp() > 0); }
      }
    }
  }
}

TEST_CASE("Relearning a talent skill after death", "[spells][death][talents]") {
  GIVEN("a talent that teaches a spell") {
    auto data = R"(
      <class name="Dancer">
          <tree name="Dancing">
              <tier>
                  <requires />
                  <talent type="spell" id="dance" />
              </tier>
          </tree>
      </class>
      <spell id="dance" name="Dance" >
        <targets self="1" />
        <function name="doDirectDamage" s1="0" />
      </spell>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("a level-2 user takes the talent") {
      user.levelUp();
      c.sendMessage(CL_CHOOSE_TALENT, "Dance");
      WAIT_UNTIL(user.getClass().knowsSpell("dance"));

      AND_WHEN("he dies") {
        user.kill();

        AND_WHEN("he tries to take the talent again") {
          c.sendMessage(CL_CHOOSE_TALENT, "Dance");

          THEN("he has the talent") {
            WAIT_UNTIL(user.getClass().knowsSpell("dance"));
          }
        }
      }
    }
  }
}

TEST_CASE("Non-damaging spells aggro NPCs", "[spells][ai]") {
  GIVEN("a debuff spell, and an NPC out of range") {
    auto data = R"(
      <spell id="exhaust" range="100" >
        <targets enemy="1" />
        <function name="debuff" s1="exhausted" />
      </spell>
      <buff id="exhausted" duration="1000" >
        <stats energy="-1" />
      </buff>
      <npcType id="monster" maxHealth="100" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    s.addNPC("monster", {200, 200});

    WHEN("a user knows the spell") {
      user.getClass().teachSpell("exhaust");

      AND_WHEN("he casts it on the NPC") {
        const auto &monster = s.getFirstNPC();
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(monster.serial()));
        c.sendMessage(CL_CAST_SPELL, "exhaust");
        WAIT_UNTIL(monster.debuffs().size() == 1);

        THEN("it is aware of him") { WAIT_UNTIL(monster.isAwareOf(user)); }
      }
    }
  }
}

TEST_CASE("Cast-from-item returning an item", "[spells][inventory]") {
  GIVEN("items that can pour water; the bucket is returned afterwards") {
    auto data = R"(
      <spell id="pourWater"  >
        <targets self="1" />
        <function name="doDirectDamage" s1="0" />
      </spell>
      <item id="bucketOfWater" castsSpellOnUse="pourWater" returnsOnCast="emptyBucket" />
      <item id="glassOfWater" castsSpellOnUse="pourWater" returnsOnCast="emptyGlass" />
      <item id="magicWaterBubble" castsSpellOnUse="pourWater" />
      <item id="emptyBucket" />
      <item id="emptyGlass" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto &invSlot = user.inventory(0);

    AND_GIVEN("a user has a bucket") {
      auto &bucketOfWater = s.findItem("bucketOfWater");
      user.giveItem(&bucketOfWater);

      WHEN("he pours out the water") {
        c.sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

        THEN("he has an empty bucket") {
          auto &emptyBucket = s.findItem("emptyBucket");
          WAIT_UNTIL(invSlot.hasItem() && invSlot.type() == &emptyBucket);
        }
      }
    }

    AND_GIVEN("a user has a magic water bubble") {
      auto &magicWaterBubble = s.findItem("magicWaterBubble");
      user.giveItem(&magicWaterBubble);

      WHEN("he pours out the water") {
        c.sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

        THEN("his inventory is empty") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(invSlot.hasItem());
        }
      }
    }

    AND_GIVEN("a user has a glass of water") {
      auto &glassOfWater = s.findItem("glassOfWater");
      user.giveItem(&glassOfWater);

      WHEN("he pours out the water") {
        c.sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

        THEN("he has an empty glass") {
          auto &emptyGlass = s.findItem("emptyGlass");
          WAIT_UNTIL(invSlot.hasItem() && invSlot.type() == &emptyGlass);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Cast-from-item with no item removal",
                 "[spells][inventory]") {
  GIVEN("a button that casts a spell without being lost") {
    useData(R"(
      <spell id="zap"  >
        <targets self="1" />
        <function name="doDirectDamage" s1="0" />
      </spell>
      <item id="button" castsSpellOnUse="zap" keepOnCast="1" />
    )");

    AND_GIVEN("the user has a button") {
      user->giveItem(&server->getFirstItem());

      WHEN("he casts with it") {
        client->sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

        THEN("he still has an item") {
          REPEAT_FOR_MS(100);
          CHECK(user->inventory(0).hasItem());
        }
      }
    }
  }
}

TEST_CASE("Cast a spell from a stackable item", "[spells][inventory]") {
  GIVEN("stackable matches cast self-fireballs and return used matches") {
    auto data = R"(
      <spell id="fireball"  >
        <targets self="1" />
        <function name="doDirectDamage" i1="10" />
      </spell>
      <item id="match" stackSize="10" castsSpellOnUse="fireball" returnsOnCast="usedMatch" />
      <item id="usedMatch" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    const auto &match = s.findItem("match");

    AND_GIVEN("a user has three matches") {
      user.giveItem(&match, 3);

      WHEN("he casts the spell") {
        c.sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

        THEN("he has two matches") {
          const auto &invSlot = user.inventory(0);
          WAIT_UNTIL(invSlot.quantity() == 2);
        }
      }
    }

    AND_GIVEN("a user has no room for used matches") {
      user.giveItem(&match, 10 * User::INVENTORY_SIZE);

      WHEN("he casts the spell") {
        c.sendMessage(CL_CAST_SPELL_FROM_ITEM, "0");

        THEN("he is still at full health") {
          REPEAT_FOR_MS(100);
          CHECK(!user.isMissingHealth());
        }
      }
    }
  }
}

TEST_CASE("Target self if target is invalid", "[spells]") {
  GIVEN("a friendly-fire fireball spell and an enemy") {
    auto data = R"(
      <spell id="fireball"  >
        <targets friendly="1" self="1" />
        <function name="doDirectDamage" i1="10" />
      </spell>
      <npcType id="distraction" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.getClass().teachSpell("fireball");
    const auto &distraction = s.addNPC("distraction", {50, 50});

    WHEN("a user targets the object") {
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(distraction.serial()));

      AND_WHEN("he tries to cast the spell") {
        c.sendMessage(CL_CAST_SPELL, "fireball");

        THEN("he himself has lost health") {
          WAIT_UNTIL(user.isMissingHealth());
        }
      }
    }
  }
}

TEST_CASE("Objects can't be healed", "[spells][damage-on-use]") {
  GIVEN("an object, and a heal-all spell") {
    auto data = R"(
      <spell id="megaHeal" range="100" >
        <targets friendly="1" self="1" enemy="1" />
        <function name="heal" i1="99999" />
      </spell>
      <item id="sprocket" durability="10" />
      <objectType id="machine">
        <durability item="sprocket" quantity="10"/>
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);
    auto &machine = s.addObject("machine", {10, 15});

    AND_GIVEN("a user knows the spell") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.getClass().teachSpell("megaHeal");

      AND_GIVEN("the object is damaged") {
        machine.reduceHealth(1);

        WHEN("the user tries to heal the object") {
          c.sendMessage(CL_SELECT_ENTITY, makeArgs(machine.serial()));
          c.sendMessage(CL_CAST_SPELL, "megaHeal");

          THEN("it is not at full health") {
            REPEAT_FOR_MS(100);
            CHECK(machine.isMissingHealth());
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "A spell that summons an NPC",
                 "[spells]") {
  GIVEN("a spell that summons an NPC") {
    useData(R"(
      <npcType id="skeleton" />
      <spell id="raiseSkeleton" >
        <targets self="1" />
        <function name="spawnNPC" s1="skeleton" />
      </spell>
    )");

    AND_GIVEN("the user knows the spell") {
      user->getClass().teachSpell("raiseSkeleton");

      WHEN("he casts it") {
        client->sendMessage(CL_CAST_SPELL, "raiseSkeleton");

        THEN("there is an NPC") { WAIT_UNTIL(server->entities().size() == 1); }
      }
    }
  }
}
