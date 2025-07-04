#include "../XmlReader.h"
#include "../client/ClientNPCType.h"
#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Read XML file with root only", "[loading]") {
  auto xr = XmlReader::FromFile("testing/empty.xml");
  for (auto elem : xr.getChildren("nonexistent_tag"))
    ;
}

TEST_CASE("No crash on bad data", "[loading]") {
  TestServer s = TestServer::WithData("this_doesnt_exist");
}

TEST_CASE("Get spawn point from map file", "[loading]") {
  TestServer s = TestServer::WithData("spawn_point_37");
  TestClient c;
  s.waitForUsers(1);
  const User &user = *s.users().begin();
  CHECK(user.location() == MapPoint(37, 37));
}

TEST_CASE("Get spawn range from map file", "[loading]") {
  TestServer s = TestServer::WithData("spawn_point_37ish");
  auto c1 = TestClient::WithData("spawn_point_37ish"),
       c2 = TestClient::WithData("spawn_point_37ish"),
       c3 = TestClient::WithData("spawn_point_37ish");

  s.waitForUsers(3);

  for (const User &user : s.users()) {
    CHECK(user.location().x > 17);
    CHECK(user.location().y > 17);
    CHECK(user.location().x < 57);
    CHECK(user.location().y < 57);
  }
}

TEST_CASE("Constructible NPC is loaded as NPC", "[loading][construction]") {
  // Load an item that refers to an object type, then an NPC type to define it
  TestClient c = TestClient::WithData("construct_an_npc");

  const ClientObjectType &objType = **c.objectTypes().begin();
  REQUIRE(objType.classTag() == 'n');

  // Check its health (to distinguish it from a plain ClientObject)
  const ClientNPCType &npcType = dynamic_cast<const ClientNPCType &>(objType);
  CHECK(npcType.maxHealth() == 5);
}

TEST_CASE("Object spawners work", "[loading][spawning]") {
  // Given a spawner that maintains 3 rocks
  // When a server runs
  TestServer s = TestServer::WithData("spawned_rocks");

  // Then there are 3 rocks
  WAIT_UNTIL(s.entities().size() == 3);
}

TEST_CASE("NPC spawners work", "[loading][spawning]") {
  // Given a spawner that maintains 3 chickens
  // When a server runs
  TestServer s = TestServer::WithData("spawned_chickens");

  // Then there are 3 chickens
  WAIT_UNTIL(s.entities().size() == 3);
}

TEST_CASE("Clients load map properly", "[loading]") {
  GIVEN(
      "a server and client, with a 101x101 map on which users spawn at the "
      "midpoint") {
    TestServer s = TestServer::WithData("big_map");
    TestClient c = TestClient::WithData("big_map");

    THEN("the client's map has 101 columns") {
      WAIT_UNTIL(c.map().width() == 101);

      AND_THEN("the user spawned in the correct place") {
        WAIT_UNTIL(c->character().location() == MapPoint(1616, 1616));
      }
    }
  }

  GIVEN("a spawn point with a non-zero range") {
    auto data = R"(
      <newPlayerSpawn range="5" />
    )";

    WHEN("a server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("the respawn range has the correct value") {
        WAIT_UNTIL(User::spawnRadius == 5);
      }
    }
  }

  GIVEN("a custom-sized map in a data string") {
    auto c = TestClient::WithDataString(R"(
      <terrain index="G" id="grass" />
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="40" y="1" />
      <row    y="0" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
    )");

    THEN("the client's map has the correct dimensions") {
      CHECK(c.map().width() == 40);
    }
  }
}

TEST_CASE("Help text is valid XML", "[loading]") {
  auto c = TestClient{};
  const auto &helpTextEntries = c->helpEntries();
  CHECK(helpTextEntries.begin() != helpTextEntries.end());
}

TEST_CASE("NPC types have correct defaults", "[loading]") {
  GIVEN("an NPC type with unspecified level") {
    auto data = R"(
      <npcType id="bacterium" />
    )";
    auto s = TestServer::WithDataString(data);

    THEN("it is level 1") {
      auto &bacterium = s.getFirstNPCType();
      CHECK(bacterium.level() == 1);

      AND_THEN("it has 1 max health") {
        CHECK(bacterium.baseStats().maxHealth == 1);
      }
    }
  }
}

TEST_CASE("NPC tags are loaded in client", "[loading]") {
  GIVEN("An NPC type with a tag") {
    auto data = R"(
      <npcType
        id="woodElf" maxHealth="1">
        <tag name="elf"/>
      </npcType>
    )";
    WHEN("The client is finished loading") {
      auto c = TestClient::WithDataString(data);
      THEN("The NPC type has tags") {
        const auto &npc = c.getFirstObjectType();
        WAIT_UNTIL(npc.hasAnyTags());
      }
    }
  }
}

TEST_CASE("Load XML from string", "[loading]") {
  GIVEN("An XML string defining an item") {
    auto data = R"(
      <item id="rock" />
    )";
    WHEN("A client is loaded with that data") {
      auto c = TestClient::WithDataString(data);
      THEN("The client has an item defined") { CHECK(c.items().size() == 1); }
    }
    WHEN("A server is loaded with that data") {
      auto s = TestServer::WithDataString(data);
      THEN("The server has an item defined") { CHECK(s.items().size() == 1); }
    }
  }
}

TEST_CASE("Clients load quests", "[loading][quests]") {
  GIVEN("a quest") {
    auto data = R"(
      <quest id="quest1" name="Quest" startsAt="a" endsAt="b"
        brief="Brief" debrief="Debrief" />
    )";

    WHEN("a client loads") {
      auto c = TestClient::WithDataString(data);

      THEN("it has a quest") { CHECK(c.quests().size() == 1); }

      AND_THEN("it has the correct data for that quest") {
        const auto &quest = c.quests().begin()->second;
        CHECK(quest.info().id == "quest1");
        CHECK(quest.info().name == "Quest");
        CHECK(quest.info().brief == "Brief");
        CHECK(quest.info().debrief == "Debrief");
      }
    }
  }
}

TEST_CASE("NPCs have default max health", "[loading][stats]") {
  GIVEN("an NPC type with only the ID defined") {
    auto data = R"(
      <npcType id="ant" />
    )";
    WHEN("a client is started") {
      auto c = TestClient::WithDataString(data);
      THEN("it has an NPC type defined") { CHECK(c.objectTypes().size() == 1); }
    }
  }
}

TEST_CASE("Existing users load at the correct location",
          "[loading][persistence]") {
  // Given Alice spawns at (10, 10)
  auto s = TestServer{};
  User *alice = nullptr;

  {
    auto c = TestClient::WithUsername("Alice");
    s.waitForUsers(1);
    alice = &s.getFirstUser();

    // When she moves to (15, 15)
    alice->location({15, 15});

    // And when she logs out then back in
  }
  {
    auto c = TestClient::WithUsername("Alice");
    s.waitForUsers(1);
    alice = &s.getFirstUser();

    // Then she is at (15,15)
    auto spawnedCorrectly = alice->location() == MapPoint{15, 15};
    CHECK(spawnedCorrectly);
  }
}

TEST_CASE("The map can be loaded from a string", "[loading]") {
  GIVEN("a 2x2 map specified by string") {
    auto data = R"(
      <size x="2" y="2" />
      <newPlayerSpawn x="10" y="10" range="50" />
      <row    y="0" terrain = "GG" />
      <row    y="1" terrain = "GG" />
    )";

    auto s = TestServer::WithDataString(data);

    THEN("The map has width=2") { CHECK(s->map().width() == 2); }
  }
}

TEST_CASE("Weapon damage school is loaded", "[loading][combat]") {
  GIVEN("a weapon that does fire damage") {
    auto data = R"(
      <item id="fireSword" >
        <weapon damage="1" speed="1" school="fire" />
      </item>
    )";

    WHEN("a server starts") {
      auto s = TestServer::WithDataString(data);

      THEN("that weapon has the correct school") {
        const auto &fireSword = s.getFirstItem();
        CHECK((fireSword.stats().weaponSchool == SpellSchool::FIRE));
      }
    }

    WHEN("a client starts") {
      auto c = TestClient::WithDataString(data);

      THEN("that weapon has the correct school") {
        const auto &fireSword = c.items().begin()->second;
        CHECK((fireSword.stats().weaponSchool == SpellSchool::FIRE));
      }
    }
  }
}

TEST_CASE("Server loads terrain types", "[loading]") {
  GIVEN("a server") {
    auto s = TestServer{};
    THEN("there's one terrain type") { CHECK(s.terrainTypes().size() == 1); }
  }
}

TEST_CASE("NPC templates", "[loading]") {
  GIVEN("an NPC that uses a template") {
    auto data = R"(
      <npcTemplate id="bear">
        <collisionRect x="1" y="2" w="3" h="4" />
      </npcTemplate>
      <npcType id="easyBear" template="bear" />
    )";
    auto s = TestServer::WithDataString(data);

    THEN("it has the correct collision info") {
      const auto &easyBear = s.getFirstNPCType();
      CHECK(easyBear.collisionRect() == MapRect{1, 2, 3, 4});
    }
  }
}

TEST_CASE("NPC forward declaration", "[loading]") {
  GIVEN("A forward-declared NPC type with a yield") {
    auto data = R"(
      <npcType id="pig" maxHealth="1" >
        <transform id="pigWithTruffle" time="1" />
      </npcType>
      <npcType id="pigWithTruffle" maxHealth="1" >
        <yield id="truffle" />
      </npcType>
      <item id="truffle" />
    )";
    auto s = TestServer::WithDataString(data);

    THEN("It actually has that yield") {
      const auto &pigWithTruffleType = *s->findObjectTypeByID("pigWithTruffle");
      CHECK(pigWithTruffleType.yield);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithDataFiles,
                 "Static objects can be listed in any file", "[loading]") {
  GIVEN("a tree defined in each of data.xml and staticObjects.xml") {
    useData("static_tree");

    THEN("two object exists on the server") {
      WAIT_UNTIL(server->entities().size() == 2);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Static objects in data string",
                 "[loading]") {
  GIVEN("a static-defined tree") {
    useData(R"(
      <objectType id="tree" />
      <object id="tree" x="20" y="20" />
    )");
    THEN("the server knows it's there") {
      WAIT_UNTIL(server->entities().size() == 1);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Permanent objects", "[loading]") {
  GIVEN("a permanent object, and a a user far away from it") {
    useData(R"(
      <newPlayerSpawn x="9000" y="10" />
      <size x="315" y="1" />
      <row y="0" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <objectType id="tree" />
      <permanentObject id="tree" x="10" y="10" />
    )");

    THEN("the server has one object") {
      WAIT_UNTIL(server->entities().size() == 1);

      AND_THEN("the user knowns about it") {
        WAIT_UNTIL_TIMEOUT(client->entities().size() == 2,
                           15000);  // User + object
        const auto &cObject = client->getFirstNonAvatarSprite();
        CHECK(cObject.location() == MapPoint{10, 10});
      }
    }
  }

  GIVEN("a permanent object with non-default health") {
    useData(R"(
      <objectType id="wall" maxHealth="100" />
      <permanentObject id="wall" x="5" y="5" />
    )");

    THEN("the entity has nonzero health") {
      const auto &cSprite = client->getFirstNonAvatarSprite();
      const auto &cObject = dynamic_cast<const ClientObject &>(cSprite);
      CHECK(cObject.health() > 0);
    }
  }

  GIVEN("a permanent object at (5,5)") {
    useData(R"(
      <objectType id="tree" />
      <permanentObject id="tree" x="5" y="5" />
    )");

    THEN("the client knows its location") {
      const auto &cObject = client->getFirstNonAvatarSprite();
      CHECK(cObject.location() == MapPoint{5, 5});
    }
  }

  GIVEN("a permanent object with missing location") {
    useData(R"(
      <objectType id="tree" />
      <permanentObject id="tree" />
    )");

    THEN("the client doesn't load it") {
      CHECK(client->entities().size() == 1);
    }
  }

  GIVEN("a permanent object with missing y co-ord") {
    useData(R"(
      <objectType id="tree" />
      <permanentObject id="tree" x="6" />
    )");

    THEN("the client doesn't load it") {
      CHECK(client->entities().size() == 1);
    }
  }

  GIVEN("a permanent object with missing type") {
    useData(R"(
      <permanentObject x="1" y="2" />
    )");

    THEN("the client doesn't load it") {
      CHECK(client->entities().size() == 1);
    }
  }

  GIVEN("a permanent object with an invalid type") {
    useData(R"(
      <permanentObject id="spook" x="1" y="2" />
    )");

    THEN("the client doesn't load it") {
      CHECK(client->entities().size() == 1);
    }
  }

  GIVEN("a different type of permanent object") {
    useData(R"(
      <objectType id="rock" />
      <permanentObject id="rock" x="10" y="10" />
    )");

    THEN("it is a rock type") {
      const auto &cObject = client->getFirstNonAvatarSprite();
      CHECK(cObject.type() == &client->getFirstObjectType());
    }
  }

  GIVEN("a permanent object close to a user") {
    useData(R"(
      <objectType id="rock" />
      <permanentObject id="rock" x="5" y="5" />
    )");
    THEN("the client doesn't have any objects") {
      REPEAT_FOR_MS(100);
      CHECK(client->objects().empty());
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithDataFiles,
                 "Clients load permanent objects from files", "[loading]") {
  GIVEN("a permanent object") {
    useData("a_permanent_object");

    THEN("the client knows about it") {
      WAIT_UNTIL(client->entities().size() == 2);
    }
  }
}

TEST_CASE("Composite stats from file", "[loading][stats]") {
  WHEN("a server starts with a composite stat in a data file") {
    auto s = TestServer::WithData("a_composite_stat");

    THEN("there's a composite stat") {
      CHECK(Stats::compositeDefinitions.size() == 1);
    }
  }

  GIVEN("a client with no data") {
    auto c = TestClient{};

    THEN("there are no composite stats") {
      CHECK(Stats::compositeDefinitions.empty());
    }
  }

  GIVEN("a server with no data") {
    auto s = TestServer{};

    WHEN("a client starts with a composite stat in a data file") {
      auto c = TestClient::WithData("a_composite_stat");

      THEN("there's a composite stat") {
        CHECK(Stats::compositeDefinitions.size() == 1);
      }
    }
  }
}

TEST_CASE("Disappearance after time", "[loading]") {
  GIVEN("ghosts disappears after 1s") {
    auto data = R"(
      <npcType id="ghost" disappearAfter="1000" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a ghost is added") {
      s.addNPC("ghost", {10, 10});

      THEN("it eventually disappears") { WAIT_UNTIL(s.entities().size() == 0); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Permanent-object decorations",
                 "[loading]") {
  auto getFirstNonUserEntity = [](TestClient *client) {
    for (auto *entity : client->entities())
      if (entity->classTag() != 'u') return entity;
    return *client->entities().begin();
  };

  GIVEN("a default object type, and a permanent instance") {
    useData(R"(
      <objectType id="rug" />
      <permanentObject id="rug" x="10" y="10" />
    )");

    THEN("the client knows it's not decorative") {
      const auto &cRug = *getFirstNonUserEntity(client);
      CHECK_FALSE(cRug.isDecoration());
    }
  }
  GIVEN("a default object type, and a permanent instance marked decorative") {
    useData(R"(
      <objectType id="rug" />
      <permanentObject id="rug" x="10" y="10" isDecoration="1" />
    )");

    THEN("the client knows it's decorative") {
      const auto &cRug = *getFirstNonUserEntity(client);
      CHECK(cRug.isDecoration());
    }
  }
  GIVEN("a decorative object type, and a default permanent instance") {
    useData(R"(
      <objectType id="rug" isDecoration="1" />
      <permanentObject id="rug" x="10" y="10" />
    )");

    THEN("the client knows it's decorative") {
      const auto &cRug = *getFirstNonUserEntity(client);
      CHECK(cRug.isDecoration());
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Permanent objects can't be interacted with", "[loading]") {
  GIVEN("a permanent instance of a container object") {
    useData(R"(
        <objectType id="box" >
          <container slots="1" />
        </objectType>
        <permanentObject id="box" x="10" y="10" />
      )");

    WHEN("the client moves the mouse over it") {
      const auto objectPositionOnScreen =
          ScreenPoint{10, 10} + ScreenPoint{2, 2} + (*client)->offset();
      client->simulateMouseMove(objectPositionOnScreen);

      THEN("it does not get highlighted") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(client->entityUnderCursor());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Item levels", "[loading]") {
  GIVEN("a default item") {
    useData(R"(
      <item id="straw" />
    )");

    THEN("it is level 0 on both server and client") {
      CHECK(server->getFirstItem().ilvl() == 0);
      CHECK(client->getFirstItem().ilvl() == 0);
    }
  }

  GIVEN("an item specified to be level 2") {
    useData(R"(
      <item id="brick" ilvl="2" />
    )");

    THEN("it is level 2 on both server and client") {
      CHECK(server->getFirstItem().ilvl() == 2);
      CHECK(client->getFirstItem().ilvl() == 2);
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Level requirements on gear",
                 "[loading]") {
  GIVEN("an item with ilvl 10") {
    useData(R"(
      <item id="hat" ilvl="10" />
    )");

    THEN("the client knows it requires level 5") {
      CHECK(client->getFirstItem().lvlReq() == 5);
    }
  }
}

TEST_CASE("Non-colliding objects load correctly", "[loading]") {
  GIVEN("an object with collides=\"0\"") {
    auto server = TestServer::WithDataString(R"(
      <objectType id="road"
          collides="0" >
          <collisionRect x="-16" y="-16" w="32" h="32" />
      </objectType>
    )");
    const auto &road = server.addObject("road", {10, 10});

    THEN("it doesn't collide") { CHECK_FALSE(road.collides()); }
  }
}
