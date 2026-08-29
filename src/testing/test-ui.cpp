#include "../client/ClientNPCType.h"
#include "../client/ui/Label.h"
#include "../client/ui/List.h"
#include "../WorkerThread.h"
#include "TestClient.h"
#include "TestFixtures.h"
#include "testing.h"
#include "TestServer.h"

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

TEST_CASE_METHOD(ServerAndClientWithData, "View merchant slots in window",
                 "[.flaky][ui][merchant]") {
  // Given vending machines have merchant slots
  useData(R"(
    <objectType id="vendingMachine" merchantSlots="2" />
  )");

  // Move user to middle
  user->moveLegallyTowards({10, 10});
  // Add a single vending machine
  server->addObject("vendingMachine", {10, 10});
  WAIT_UNTIL(server->entities().size() == 1);
  WAIT_UNTIL(client->objects().size() == 1);

  auto objects = client->objects();
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
  client->waitForRedraw();
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "New client can build default constructions",
                 "[ui][construction]") {
  // Given a buildable brick wall object type with no pre-requisites
  useData(R"(
    <item id="brick" />
    <objectType id="wall" constructionTime="0" >
      <material id="brick" quantity="1" />
    </objectType>
  )");

  // His construction window contains at least one item
  CHECK_FALSE(client->uiBuildList().empty());
}

TEST_CASE_METHOD(ServerAndClient, "New client has target UI hidden", "[ui]") {
  // Then his targeting UI is hidden
  CHECK_FALSE(client.target().panel()->visible());
}

TEST_CASE_METHOD(ServerAndClient, "Chat messages are added to chat log",
                 "[ui][chat]") {
  // When a client sends a message
  client.sendMessage(CL_SAY, "asdf");

  // Then his chat log contains at least one message
  WAIT_UNTIL(client.chatLog()->size() > 0);
}

TEST_CASE_METHOD(ServerAndClient, "Windows start uninitialized", "[ui]") {
  // Then the crafting window is uninitialized
  CHECK_FALSE(client.craftingWindow()->isInitialized());
}

TEST_CASE_METHOD(ServerAndClient, "Windows are initialized when used", "[ui]") {
  // When the client opens the crafting window
  client.craftingWindow()->show();

  // Then it is initializezd
  WAIT_UNTIL(client.craftingWindow()->isInitialized());
}

TEST_CASE_METHOD(ServerAndClient, "A visible window is fully-formed", "[ui]") {
  // When a client opens the build window
  client.buildWindow()->show();

  // Then the build window has dimensions;
  WAIT_UNTIL(client.buildWindow()->Element::width() > 0);

  // And the heading has a texture
  WAIT_UNTIL(client.buildWindow()->getHeading()->texture());
}

TEST_CASE("Element gets initialized with Client", "[.flaky][ui]") {
  CHECK_FALSE(Element::isInitialized());  // Depends on test order.
  Client c;
  WAIT_UNTIL(Element::isInitialized());
}

TEST_CASE_METHOD(ServerAndClient, "Gear window can be viewed", "[gear][ui]") {
  client.gearWindow()->show();
  WAIT_UNTIL(client.gearWindow()->texture());
}

TEST_CASE_METHOD(ServerAndClientWithData, "New clients survive recipe unlocks",
                 "[ui][crafting][unlocking]") {
  useData(R"(
    <item id="flour" />
    <item id="bread" />
    <recipe id="bread" time="10" product="bread" >
      <material id="flour" />
      <unlockedBy item="flour" />
    </recipe>
  )");

  // When the server alerts the client to a recipe unlock
  user->sendMessage({SV_NEW_RECIPES_LEARNED, makeArgs(1, "asdf")});

  // The client receives it.
  CHECK(client->waitForMessageEver(SV_NEW_RECIPES_LEARNED));
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

TEST_CASE_METHOD(ServerAndClientWithData,
                 "A player's objects are the appropriate color",
                 "[permissions][ui]") {
  useData(R"(
    <objectType id="rock" />
  )");

  server->addObject("rock", {10, 15}, user->name());

  WAIT_UNTIL(client->objects().size() == 1);
  const auto &rock = client->getFirstObject();
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

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Object windows close if they change to allow only demolition",
                 "[ui][quests]") {
  GIVEN("an object with a quest, owned by a user") {
    useData(R"(
      <objectType id="A" />
      <objectType id="B" />
      <quest id="quest1" startsAt="A" endsAt="B" />
    )");
    server->addObject("A", {10, 15}, user->name());

    WHEN("he opens the object's window") {
      WAIT_UNTIL(client->objects().size() == 1);
      auto &cObject = client->getFirstObject();
      // Wait, to avoid concurrent calls to ClientObject::assembleWindow()
      REPEAT_FOR_MS(100);
      cObject.onRightClick();
      REQUIRE(cObject.window());
      CHECK(cObject.window()->visible());

      AND_WHEN("he accepts the quest") {
        client->sendMessage(CL_ACCEPT_QUEST,
                            makeArgs("quest1", cObject.serial()));

        THEN("the window closes") { WAIT_UNTIL(!cObject.window()->visible()); }
      }
    }
  }

  GIVEN("an object owned by a user") {
    useData(R"(
      <objectType id="A" />
    )");
    server->addObject("A", {10, 15}, user->name());

    WHEN("he opens the object's window") {
      WAIT_UNTIL(client->objects().size() == 1);
      auto &cObject = client->getFirstObject();
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
