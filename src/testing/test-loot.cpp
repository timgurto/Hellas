#include "../client/ui/TakeContainer.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Client gets loot info and can loot", "[loot]") {
  // Given an NPC that always drops 1 gold
  TestServer s = TestServer::WithData("goldbug");
  s.addNPC("goldbug", {10, 15});

  // When a user kills it
  TestClient c = TestClient::WithData("goldbug");
  WAIT_UNTIL(c.objects().size() == 1);
  NPC &goldbug = s.getFirstNPC();
  c.sendMessage(CL_TARGET_ENTITY, makeArgs(goldbug.serial()));
  WAIT_UNTIL(goldbug.health() == 0);

  // Then the user can see one item in its loot window;
  ClientNPC &clientGoldbug = c.getFirstNPC();
  WAIT_UNTIL(clientGoldbug.container().size() == 1);

  // And the server survives a loot request;
  c.sendMessage(CL_TAKE_ITEM, makeArgs(goldbug.serial(), 0));

  // And the client receives the item
  WAIT_UNTIL(c.inventory()[0].first.type != nullptr);
}

TEST_CASE("Objects have health", "[strength]") {
  // Given a running server;
  // And a chair object type with the strength of 6 wood;
  // And a wood item with 5 health
  TestServer s = TestServer::WithData("chair");

  // When a chair object is created
  s.addObject("chair");

  // It has 30 (6*5) health
  CHECK(s.getFirstObject().health() == 30);
}

TEST_CASE("Clients discern NPCs with no loot", "[loot]") {
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

TEST_CASE("Chance for strength-items as loot from object",
          "[loot][strength][.flaky]") {
  // Given a running server and client;
  // And a snowflake item with 1 health;
  // And a snowman object type made of 1000 snowflakes;
  TestServer s = TestServer::WithData("snowman");
  TestClient c = TestClient::WithData("snowman");
  s.waitForUsers(1);

  // And a snowman exists
  s.addObject("snowman", {10, 15});

  // When the snowman is destroyed
  Object &snowman = s.getFirstObject();
  snowman.reduceHealth(9999);

  // Then the client finds out that it's lootable
  WAIT_UNTIL(c.objects().size() == 1);
  ClientObject &clientSnowman = c.getFirstObject();
  c.waitForMessage(SV_INVENTORY);

  WAIT_UNTIL(clientSnowman.lootable());

  SECTION("Looting works") {
    WAIT_UNTIL(clientSnowman.container().size() > 0);

    c.sendMessage(CL_TAKE_ITEM, makeArgs(snowman.serial(), 0));
    WAIT_UNTIL(c.inventory()[0].first.type != nullptr);
  }

  SECTION("The loot window works") {
    // When he right-clicks on the chest
    clientSnowman.onRightClick(c.client());

    // And the loot window appears
    WAIT_UNTIL(clientSnowman.lootContainer() != nullptr);
    WAIT_UNTIL(clientSnowman.lootContainer()->size() > 0);

    // Then the user can loot using this window
    ScreenPoint buttonPos =
        clientSnowman.window()->rect() + ScreenRect{0, Window::HEADING_HEIGHT} +
        clientSnowman.lootContainer()->rect() + ScreenRect{5, 5};
    c.simulateClick(buttonPos);

    WAIT_UNTIL(c.inventory()[0].first.type != nullptr);
  }
}

TEST_CASE("Looting from a container", "[loot][container][only][.flaky]") {
  // Given a running server and client;
  // And a chest object type with 10 container slots;
  // And a gold item that stacks to 100;
  TestServer s = TestServer::WithData("chest_of_gold");
  TestClient c = TestClient::WithData("chest_of_gold");
  s.waitForUsers(1);

  // And a chest exists;
  s.addObject("chest", {10, 15});

  auto &chest = s.getFirstObject();
  const auto &gold = s.getFirstItem();

  SECTION("Some container contents can be looted") {
    // And the chest is full of gold;
    chest.container().addItems(&gold, 1000);

    // And the chest is destroyed
    chest.reduceHealth(9999);
    REQUIRE_FALSE(chest.loot().empty());
    REQUIRE(c.waitForMessage(SV_INVENTORY));

    auto &clientChest = c.getFirstObject();
    WAIT_UNTIL(clientChest.lootable());

    SECTION("Users can receive loot") {
      // When he loots every slot
      for (size_t i = 0; i != 10; ++i)
        c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), i));

      // Then he gets some gold;
      WAIT_UNTIL(c.inventory()[0].first.type != nullptr);

      // And he doesn't get all 1000
      WAIT_UNTIL(chest.container().isEmpty());
      ItemSet thousandGold;
      thousandGold.add(&gold, 1000);
      CHECK_FALSE(s.getFirstUser().hasItems(thousandGold));
    }

    SECTION("The loot window works") {
      // When he right-clicks on the chest
      clientChest.onRightClick(c.client());

      // Then it shows the loot window;
      WAIT_UNTIL(clientChest.lootContainer() != nullptr);

      // And the window has volume;
      CHECK(clientChest.window()->contentWidth() > 0);

      // And at least one item is listed;
      WAIT_UNTIL(clientChest.lootContainer()->size() > 0);

      // And the user can loot using this window
      ScreenPoint buttonPos =
          clientChest.window()->rect() + ScreenRect{0, Window::HEADING_HEIGHT} +
          clientChest.lootContainer()->rect() + ScreenRect{5, 5};
      c.simulateClick(buttonPos);

      WAIT_UNTIL(c.inventory()[0].first.type != nullptr);
    }
  }

  SECTION("An empty container yields no loot") {
    // When the chest is destroyed
    chest.reduceHealth(9999);

    // Then he can't loot it
    REPEAT_FOR_MS(200);
    c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), 0));
    REPEAT_FOR_MS(200);
    CHECK(c.inventory()[0].first.type == nullptr);
  }

  SECTION("An owned container can be looted from") {
    // And the chest has an owner
    chest.permissions().setPlayerOwner("Alice");

    // And the chest contains gold;
    chest.container().addItems(&gold, 100);

    // And the chest is destroyed
    chest.reduceHealth(9999);
    REQUIRE_FALSE(chest.loot().empty());

    // When he loots it
    c.waitForMessage(SV_INVENTORY);
    c.sendMessage(CL_TAKE_ITEM, makeArgs(chest.serial(), 0));

    // Then he gets some gold
    WAIT_UNTIL(c.inventory()[0].first.type != nullptr);
  }
}

TEST_CASE("New users are alerted to lootable objects", "[loot]") {
  // Given a running server;
  // And a snowflake item with 1 health;
  // And a snowman object type made of 1000 snowflakes;
  TestServer s = TestServer::WithData("snowman");

  // And a snowman exists;
  s.addObject("snowman", {10, 15});

  // And the snowman is dead
  Object &snowman = s.getFirstObject();
  snowman.reduceHealth(9999);

  // When a client logs in
  TestClient c = TestClient::WithData("snowman");
  s.waitForUsers(1);

  // Then the client finds out that it's lootable
  CHECK(c.waitForMessage(SV_INVENTORY));
}

TEST_CASE("Non-taggers are not alerted to lootable objects", "[loot]") {
  // Given a running server;
  // And a snowflake item with 1 health;
  // And a snowman object type made of 1000 snowflakes;
  TestServer s = TestServer::WithData("snowman");
  TestClient c = TestClient::WithData("snowman");
  s.waitForUsers(1);

  // And a snowman exists;
  s.addObject("snowman", {10, 15});

  // And the snowman dies without the user killing it
  Object &snowman = s.getFirstObject();
  snowman.reduceHealth(9999);

  // Then the client doesn't finds out that it's lootable
  CHECK_FALSE(c.waitForMessage(SV_INVENTORY));
}
