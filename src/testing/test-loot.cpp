#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Client gets loot info and can loot", "[loot]"){
    // Given an NPC that always drops 1 gold
    TestServer s = TestServer::WithData("goldbug");
    s.addNPC("goldbug", Point(10, 15));

    // When a user kills it
    TestClient c = TestClient::WithData("goldbug");
    WAIT_UNTIL(c.objects().size() == 1);
    NPC &goldbug = s.getFirstNPC();
    c.sendMessage(CL_TARGET_ENTITY, makeArgs(goldbug.serial()));
    WAIT_UNTIL(goldbug.health() == 0);

    // Then the user can see one item in its the loot window;
    ClientNPC &clientGoldbug = c.getFirstNPC();
    c.watchObject(clientGoldbug);
    WAIT_UNTIL(clientGoldbug.container().size() == 1);

    // And the server survives a loot request;
    c.sendMessage(CL_TAKE_ITEM, makeArgs(goldbug.serial(), 0));

    // And the client receives the item
    WAIT_UNTIL(c.inventory()[0].first != nullptr);
}

TEST_CASE("Objects have health", "[strength]"){
    // Given a running server;
    // And a chair object type with the strength of 6 wood;
    // And a wood item with 5 health
    TestServer s = TestServer::WithData("chair");

    // When a chair object is created
    s.addObject("chair");

    // It has 30 (6*5) health
    CHECK(s.getFirstObject().health() == 30);
}

TEST_CASE("Clients discern NPCs with no loot", "[loot]"){
    // Given a server and client;
    // And an ant NPC type with 1 health and no loot table
    TestServer s = TestServer::WithData("ant");
    TestClient c = TestClient::WithData("ant");

    // And an ant NPC exists
    s.addNPC("ant");
    WAIT_UNTIL(c.objects().size() == 1);

    // When the ant dies
    NPC &serverAnt = s.getFirstNPC();
    serverAnt.reduceHealth(1);

    // The user doesn't believe he can loot it
    ClientNPC &clientAnt = c.getFirstNPC();
    REPEAT_FOR_MS(200);
    CHECK_FALSE(clientAnt.lootable());
}

TEST_CASE("Chance for strength-items as loot from object", "[loot][strength]"){
    // Given a running server and client;
    // And a snowflake item with 1 health;
    // And a snowman object type made of 1000 snowflakes;
    TestServer s = TestServer::WithData("snowman");
    TestClient c = TestClient::WithData("snowman");
    WAIT_UNTIL(s.users().size() == 1);

    // And a snowman exists
    s.addObject("snowman", Point(10, 15));

    // When the snowman is destroyed
    Object &snowman = s.getFirstObject();
    snowman.reduceHealth(9999);

    // Then the client finds out that it's lootable
    WAIT_UNTIL(c.objects().size() == 1);
    ClientObject &clientSnowman = c.getFirstObject();
    c.waitForMessage(SV_LOOTABLE);

    CHECK(clientSnowman.lootable());

    c.watchObject(clientSnowman);
    WAIT_UNTIL(clientSnowman.container().size() > 0);

    c.sendMessage(CL_TAKE_ITEM, makeArgs(snowman.serial(), 0));
    WAIT_UNTIL(c.inventory()[0].first != nullptr);
}

TEST_CASE("Looting from a container", "[loot][container]"){
    // Given a running server and client;
    // And a chest object type with 10 container slots;
    // And a gold item that stacks to 100;
    TestServer s = TestServer::WithData("chest_of_gold");
    TestClient c = TestClient::WithData("chest_of_gold");
    WAIT_UNTIL(s.users().size() == 1);

    // And a chest exists;
    s.addObject("chest", Point(10, 15));
        auto &chest = s.getFirstObject();

    SECTION("Container contents can be looted"){

        // And the chest is full of gold;
        const auto &gold = s.getFirstItem();
        chest.container().addItems(&gold, 1000);

        // And the chest is destroyed
        chest.reduceHealth(9999);
        REQUIRE_FALSE(chest.loot().empty());

        // When he loots every slot
        c.waitForMessage(SV_LOOTABLE);
        for (size_t i = 0; i != 10; ++i)
            c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), i));

        // Then he gets some gold;
        WAIT_UNTIL(c.inventory()[0].first != nullptr);

        // And he doesn't get all of it
        WAIT_UNTIL(chest.container().isEmpty());
        ItemSet thousandGold;
        thousandGold.add(&gold, 1000);
        CHECK_FALSE(s.getFirstUser().hasItems(thousandGold));
    }

    SECTION("An empty container yields no loot"){
        // When the chest is destroyed
        chest.reduceHealth(9999);

        // Then he can't loot it
        REPEAT_FOR_MS(200);
        c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), 0));
        REPEAT_FOR_MS(200);
        CHECK(c.inventory()[0].first == nullptr);
    }
}
