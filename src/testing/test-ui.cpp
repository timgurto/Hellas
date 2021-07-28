#include "../WorkerThread.h"
#include "../client/ClientNPCType.h"
#include "../client/ui/Label.h"
#include "../client/ui/List.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

extern WorkerThread SDLThread;

TEST_CASE("Size of empty list", "[ui]") {
  // When a new List element is created
  Element::initialize();
  List l({0, 0, 100, 100});

  // Then its size is 0
  CHECK(l.size() == 0);
}

TEST_CASE("Size of nonempty list", "[ui]") {
  // Given an empty List element
  Element::initialize();
  List l({0, 0, 100, 100});

  // When an element is added
  l.addChild(new Label({}, "asdf"));

  // Then its size is 1
  CHECK(l.size() == 1);
}

TEST_CASE("List is resized on new child", "[ui]") {
  GIVEN("an empty List") {
    Element::initialize();
    auto l = List{{0, 0, 100, 100}};

    WHEN("an element is added") {
      l.addChild(new Label({}, "asdf"));

      THEN("its content grows in height") { CHECK(l.contentHeight() > 0); }
    }
  }
}

TEST_CASE("View merchant slots in window", "[.flaky][ui][merchant]") {
  // Given a logged-in client and an object with merchant slots
  TestServer s = TestServer::WithData("merchant");
  TestClient c = TestClient::WithData("merchant");
  // Move user to middle
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  user.moveLegallyTowards({10, 10});
  // Add a single vending machine
  s.addObject("vendingMachine", {10, 10});
  WAIT_UNTIL(s.entities().size() == 1);
  WAIT_UNTIL(c.objects().size() == 1);

  auto objects = c.objects();
  auto it = objects.begin();
  auto serial = it->first;
  ClientObject *cObj = it->second;
  REQUIRE(cObj != nullptr);

  // When the client opens the object's window
  cObj->onRightClick();
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

TEST_CASE("New client can build default constructions", "[ui][construction]") {
  // Given a buildable brick wall object type with no pre-requisites
  TestServer s = TestServer::WithData("brick_wall");

  // When a client logs in
  TestClient c = TestClient::WithData("brick_wall");
  s.waitForUsers(1);

  // His construction window contains at least one item
  CHECK_FALSE(c.uiBuildList().empty());
}

TEST_CASE("New client has target UI hidden", "[ui]") {
  // When a client logs in
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // Then his targeting UI is hidden
  CHECK_FALSE(c.target().panel()->visible());
}

TEST_CASE("Chat messages are added to chat log", "[ui][chat]") {
  // Given a logged-in client
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // When he sends a message
  c.sendMessage(CL_SAY, "asdf");

  // Then his chat log contains at least one message
  WAIT_UNTIL(c.chatLog()->size() > 0);
}

TEST_CASE("Windows start uninitialized", "[ui]") {
  // Given a server and client
  TestServer s;
  TestClient c;

  // When the client logs in
  s.waitForUsers(1);

  // Then the crafting window is uninitialized
  CHECK_FALSE(c.craftingWindow()->isInitialized());
}

TEST_CASE("Windows are initialized when used", "[ui]") {
  // Given a server and client
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // When the client opens the crafting window
  c.craftingWindow()->show();

  // Then it is initializezd
  WAIT_UNTIL(c.craftingWindow()->isInitialized());
}

TEST_CASE("A visible window is fully-formed", "[ui]") {
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

TEST_CASE("Element gets initialized with Client", "[.flaky][ui]") {
  CHECK_FALSE(Element::isInitialized());  // Depends on test order.
  Client c;
  WAIT_UNTIL(Element::isInitialized());
}

TEST_CASE("Gear window can be viewed", "[gear][ui]") {
  TestServer s;
  TestClient c;
  c.gearWindow()->show();
  WAIT_UNTIL(c.gearWindow()->texture());
}

TEST_CASE("New clients survive recipe unlocks", "[ui][crafting][unlocking]") {
  // Given a client and server
  TestServer s = TestServer::WithData("secret_bread");
  TestClient c = TestClient::WithData("secret_bread");
  s.waitForUsers(1);

  // When the server alerts the client to a recipe unlock
  s.getFirstUser().sendMessage({SV_NEW_RECIPES_LEARNED, makeArgs(1, "asdf")});

  // The client receives it.
  CHECK(c.waitForMessage(SV_NEW_RECIPES_LEARNED));
}

TEST_CASE("Gear-slot names are initialized once", "[.slow][gear][ui]") {
  {
    TestClient c;
    WAIT_UNTIL(!Client::GEAR_SLOT_NAMES.empty());
  }
  {
    TestClient c2;
    CHECK(Client::GEAR_SLOT_NAMES.size() == 8);
  }
}

TEST_CASE("A player's objects are the appropriate color", "[permissions][ui]") {
  TestServer s = TestServer::WithData("basic_rock");
  TestClient c = TestClient::WithUsernameAndData("Alice", "basic_rock");

  s.addObject("rock", {10, 15}, "Alice");

  WAIT_UNTIL(c.objects().size() == 1);
  const auto &rock = c.getFirstObject();
  WAIT_UNTIL(rock.nameColor() == Color::COMBATANT_SELF);
}

TEST_CASE("Word wrapper", "[ui]") {
  GIVEN("a word wrapper") {
    TTF_Font *font = TTF_OpenFont("AdvoCut.ttf", 10);
    auto ww = WordWrapper(font, 200);

    WHEN("it's given two lines of input with two words each") {
      auto input = R"(
        line 1
        line 2
      )";
      auto output = ww.wrap(input);

      THEN("the first line is together on a line") {
        CHECK(output[0] == "line 1");
      }
    }
    TTF_CloseFont(font);
  }
}

TEST_CASE("Object windows close if they change to allow only demolition",
          "[ui][quests]") {
  GIVEN("an object with a quest, owned by a user") {
    auto data = R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="quest1" startsAt="A" endsAt="B" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    const auto &user = s.getFirstUser();
    s.addObject("A", {10, 15}, user.name());

    WHEN("he opens the object's window") {
      WAIT_UNTIL(c.objects().size() == 1);
      auto &cObject = c.getFirstObject();
      // Wait, to avoid concurrent calls to ClientObject::assembleWindow()
      REPEAT_FOR_MS(100);
      cObject.onRightClick();
      REQUIRE(cObject.window());
      CHECK(cObject.window()->visible());

      AND_WHEN("he accepts the quest") {
        c.sendMessage(CL_ACCEPT_QUEST, makeArgs("quest1", cObject.serial()));

        THEN("the window closes") { WAIT_UNTIL(!cObject.window()->visible()); }
      }
    }
  }

  GIVEN("an object owned by a user") {
    auto data = R"(
      <objectType id="A" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    const auto &user = s.getFirstUser();
    s.addObject("A", {10, 15}, user.name());

    WHEN("he opens the object's window") {
      WAIT_UNTIL(c.objects().size() == 1);
      auto &cObject = c.getFirstObject();
      // Wait, to avoid concurrent calls to ClientObject::assembleWindow()
      REPEAT_FOR_MS(100);
      cObject.onRightClick();

      THEN("it is visible") {
        REQUIRE(cObject.window());
        CHECK(cObject.window()->visible());
      }
    }
  }
}

TEST_CASE("Short time display", "[ui]") {
  CHECK(sAsShortTimeDisplay(1) == "1s");
  CHECK(sAsShortTimeDisplay(2) == "2s");
  CHECK(sAsShortTimeDisplay(60) == "60s");
  CHECK(sAsShortTimeDisplay(61) == "1m");
  CHECK(sAsShortTimeDisplay(120) == "2m");
  CHECK(sAsShortTimeDisplay(3601) == "1h");
}
