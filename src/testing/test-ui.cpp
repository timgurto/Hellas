#include "../client/ClientNPCType.h"
#include "../client/ui/Label.h"
#include "../client/ui/List.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Size of empty list") {
  // When a new List element is created
  Element::initialize();
  List l({0, 0, 100, 100});

  // Then its size is 0
  CHECK(l.size() == 0);
}

TEST_CASE("Size of nonempty list") {
  // Given an empty List element
  Element::initialize();
  List l({0, 0, 100, 100});

  // When an element is added
  l.addChild(new Label({}, "asdf"));

  // Then its size is 1
  CHECK(l.size() == 1);
}

TEST_CASE("View merchant slots in window", "[.flaky]") {
  // Given a logged-in client and an object with merchant slots
  TestServer s = TestServer::WithData("merchant");
  TestClient c = TestClient::WithData("merchant");
  // Move user to middle
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  user.updateLocation({10, 10});
  // Add a single vending machine
  s.addObject("vendingMachine", {10, 10});
  WAIT_UNTIL(s.entities().size() == 1);
  WAIT_UNTIL(c.objects().size() == 1);

  auto objects = c.objects();
  auto it = objects.begin();
  size_t serial = it->first;
  ClientObject *cObj = it->second;
  REQUIRE(cObj != nullptr);

  // When the client opens the object's window
  cObj->onRightClick(c.client());
  WAIT_UNTIL(cObj->window() != nullptr);
  // Wait until the merchant interface is drawn
  typedef const Element *ep_t;
  const ep_t &e = cObj->merchantSlotElements()[0];
  WAIT_UNTIL(e != nullptr);
  WAIT_UNTIL(e->changed() == false);
  // Wait until merchant-slot details are received from server, and the element
  // constructed
  WAIT_UNTIL(e->children().size() > 0);

  // Then the client successfully redraws without crashing
  c.waitForRedraw();
}

TEST_CASE("New client can build default constructions") {
  // Given a buildable brick wall object type with no pre-requisites
  TestServer s = TestServer::WithData("brick_wall");

  // When a client logs in
  TestClient c = TestClient::WithData("brick_wall");
  s.waitForUsers(1);

  // His construction window contains at least one item
  CHECK_FALSE(c.uiBuildList().empty());
}

TEST_CASE("New client has target UI hidden") {
  // When a client logs in
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // Then his targeting UI is hidden
  CHECK_FALSE(c.target().panel()->visible());
}

TEST_CASE("Chat messages are added to chat log") {
  // Given a logged-in client
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // When he sends a message
  c.sendMessage(CL_SAY, "asdf");

  // Then his chat log contains at least one message
  WAIT_UNTIL(c.chatLog()->size() > 0);
}

TEST_CASE("Windows start uninitialized") {
  // Given a server and client
  TestServer s;
  TestClient c;

  // When the client logs in
  s.waitForUsers(1);

  // Then the crafting window is uninitialized
  CHECK_FALSE(c.craftingWindow()->isInitialized());
}

TEST_CASE("Windows are initialized when used") {
  // Given a server and client
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // When the client opens the crafting window
  c.craftingWindow()->show();

  // Then it is initializezd
  WAIT_UNTIL(c.craftingWindow()->isInitialized());
}

TEST_CASE("A visible window is fully-formed") {
  // Given a server and client;
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // When the client opens the build window
  c.buildWindow()->show();

  // Then the build window has dimensions;
  WAIT_UNTIL(c.buildWindow()->Element::width() > 0);

  // And the heading has a texture
  WAIT_UNTIL(c.buildWindow()->getHeading()->texture());
}

TEST_CASE("Element gets initialized with Client", "[.flaky]") {
  CHECK_FALSE(Element::isInitialized());  // Depends on test order.
  Client c;
  WAIT_UNTIL(Element::isInitialized());
}

TEST_CASE("Gear window can be viewed", "[gear]") {
  TestServer s;
  TestClient c;
  c.gearWindow()->show();
  WAIT_UNTIL(c.gearWindow()->texture());
}

TEST_CASE("New clients survive recipe unlocks", "") {
  // Given a client and server
  TestServer s = TestServer::WithData("secret_bread");
  TestClient c = TestClient::WithData("secret_bread");
  s.waitForUsers(1);

  // When the server alerts the client to a recipe unlock
  s.sendMessage(s.getFirstUser().socket(), SV_NEW_RECIPES, makeArgs(1, "asdf"));

  // The client receives it.
  CHECK(c.waitForMessage(SV_NEW_RECIPES));
}

TEST_CASE("Gear-slot names are initialized once",
          "[.slow][tooltip][gear][inventory]") {
  {
    TestClient c;
    WAIT_UNTIL(!Client::GEAR_SLOT_NAMES.empty());
  }
  {
    TestClient c2;
    CHECK(Client::GEAR_SLOT_NAMES.size() == 8);
  }
}

TEST_CASE("A player's objects are the appropriate color",
          "[color][ownership]") {
  TestServer s = TestServer::WithData("basic_rock");
  TestClient c = TestClient::WithUsernameAndData("Alice", "basic_rock");

  s.addObject("rock", {10, 15}, "Alice");

  WAIT_UNTIL(c.objects().size() == 1);
  const auto &rock = c.getFirstObject();
  WAIT_UNTIL(rock.nameColor() == Color::COMBATANT_SELF);
}
