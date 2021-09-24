#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Object container empty check", "[containers]") {
  TestServer s;
  ObjectType type("box");
  type.addContainer(ContainerType::WithSlots(5));
  Object obj(&type, {});
  CHECK(obj.container().isEmpty());
  ServerItem item("rock");
  obj.container().at(1) = {
      &item, ServerItem::Instance::ReportingInfo::InObjectContainer(), 1};
  CHECK_FALSE(obj.container().isEmpty());
}

TEST_CASE("Dismantle an object with an inventory", "[.flaky][containers]") {
  // Given a running server;
  TestServer s = TestServer::WithData("dismantle");
  // And a user at (10, 10);
  TestClient c = TestClient::WithData("dismantle");
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  user.moveLegallyTowards({10, 10});
  // And a box at (10, 10) that is deconstructible and has an empty inventory
  const auto &box = s.addObject("box", {10, 10});
  WAIT_UNTIL(c.objects().size() == 1);

  // When the user tries to deconstruct the box
  c.sendMessage(CL_PICK_UP_OBJECT_AS_ITEM, makeArgs(box.serial()));

  // The deconstruction action successfully begins
  CHECK(c.waitForMessage(SV_ACTION_STARTED));
}

TEST_CASE("Place item in object", "[.flaky][containers]") {
  TestServer s = TestServer::WithData("dismantle");
  TestClient c = TestClient::WithData("dismantle");

  // Add a single box
  const auto &box = s.addObject("box", {10, 10});
  WAIT_UNTIL(c.objects().size() == 1);

  // Give user a box item
  User &user = s.getFirstUser();
  user.giveItem(&*s.items().begin());
  CHECK(c.waitForMessage(SV_INVENTORY));

  // Try to put item in object
  c.sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Gear(), 0, box.serial(), 0));

  // Should be the alert that the object's inventory has changed
  CHECK(c.waitForMessage(SV_INVENTORY));
}

TEST_CASE("Client-side containers don't spontaneously clear", "[containers]") {
  // Given a server and client, and a "box" container object,
  auto s = TestServer::WithData("dismantle");
  auto c = TestClient::WithData("dismantle");
  s.waitForUsers(1);
  auto username = c.name();

  // And a single box belonging to the user
  s.addObject("box", {10, 10}, username);
  WAIT_UNTIL(c.objects().size() == 1);

  // When some time passes
  REPEAT_FOR_MS(100);

  // Then the client-side box still has container slots
  CHECK_FALSE(c.getFirstObject().container().empty());
}

TEST_CASE("Merchant can use same slot for ware and price",
          "[containers][merchant]") {
  GIVEN("a merchant object with one inventory slot, containing the ware") {
    auto data = R"(
        <item id="diamond" />
        <item id="coin" />
        <objectType id="diamondStore" merchantSlots="1" >
          <container slots="1" />
        </objectType>
      )";
    auto s = TestServer::WithDataString(data);
    s.addObject("diamondStore", {10, 15});
    auto &store = s.getFirstObject();
    const auto *diamond = s->findItem("diamond");
    const auto *coin = s->findItem("coin");
    store.merchantSlot(0) = {diamond, 1, coin, 1};
    store.container().addItems(diamond);

    WHEN("a user with the price tries to buy the ware") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.giveItem(coin);

      c.sendMessage(CL_TRADE, makeArgs(store.serial(), 0));

      THEN("he has the ware") {
        const auto &invSlot = user.inventory(0);
        WAIT_UNTIL(invSlot.type() == diamond);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Bad inventory message to client",
                 "[containers]") {
  GIVEN("a server and client, and an item type") {
    useData(R"(
      <item id="gold" />
    )");

    WHEN("the server sends SV_INVENTORY with a bad serial") {
      const auto badSerial = 50;
      user->sendMessage(
          {SV_INVENTORY, makeArgs(badSerial, 0, "gold"s, 1, 1, ""s)});

      THEN("the client survives") { REPEAT_FOR_MS(100); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Taking items",
                 "[containers][inventory]") {
  GIVEN("a box with a chocolate inside") {
    useData(R"(
      <item id="chocolate" />
      <objectType id="box" >
        <container slots="1"/>
      </objectType>
    )");
    auto &box = server->addObject("box", {15, 15}, user->name());
    box.container().addItems(&server->getFirstItem());

    WHEN("a user sends CL_TAKE_ITEM") {
      client->sendMessage(CL_TAKE_ITEM, makeArgs(box.serial(), 0));

      THEN("he has an item") { WAIT_UNTIL(user->inventory(0).hasItem()); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Small stacks are consumed first",
                 "[inventory") {
  GIVEN("an item that stacks to 5") {
    useData(R"(
      <item id="coin" stackSize="5" />
    )");
    const auto *coin = &server->getFirstItem();

    AND_GIVEN("the user has 7 of them (a 5 stack and a 2 stack)") {
      user->giveItem(coin, 7);

      WHEN("one is removed") {
        auto itemsToRemove = ItemSet{};
        itemsToRemove.add(coin, 1);
        user->removeItems(itemsToRemove);

        THEN("he still has the entire 5 stack") {
          CHECK(user->inventory(0).quantity() == 5);
        }
      }
    }

    SECTION("All slots are counted if they are reordered") {
      AND_GIVEN("the user has 10 of them (two full stacks)") {
        user->giveItem(coin, 10);

        WHEN("6 are removed") {
          auto itemsToRemove = ItemSet{};
          itemsToRemove.add(coin, 6);
          user->removeItems(itemsToRemove);

          THEN("he has 4 left") {
            const auto qtyInSlot0 = user->inventory(0).quantity();
            const auto qtyInSlot1 = user->inventory(1).quantity();
            CHECK(qtyInSlot0 + qtyInSlot1 == 4);
          }
        }
      }
    }
  }
}
