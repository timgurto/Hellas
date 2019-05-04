#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Object container empty check", "[container]") {
  TestServer s;
  ObjectType type("box");
  type.addContainer(ContainerType::WithSlots(5));
  Object obj(&type, {});
  CHECK(obj.container().isEmpty());
  ServerItem item("rock");
  obj.container().at(1) = std::make_pair(&item, 1);
  CHECK_FALSE(obj.container().isEmpty());
}

TEST_CASE("Dismantle an object with an inventory", "[.flaky][container]") {
  // Given a running server;
  TestServer s = TestServer::WithData("dismantle");
  // And a user at (10, 10);
  TestClient c = TestClient::WithData("dismantle");
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  user.updateLocation({10, 10});
  // And a box at (10, 10) that is deconstructible and has an empty inventory
  s.addObject("box", {10, 10});
  WAIT_UNTIL(c.objects().size() == 1);

  // When the user tries to deconstruct the box
  size_t serial = c.objects().begin()->first;
  c.sendMessage(CL_DECONSTRUCT, makeArgs(serial));

  // The deconstruction action successfully begins
  CHECK(c.waitForMessage(SV_ACTION_STARTED));
}

TEST_CASE("Place item in object", "[.flaky][container]") {
  TestServer s = TestServer::WithData("dismantle");
  TestClient c = TestClient::WithData("dismantle");

  // Add a single box
  s.addObject("box", {10, 10});
  WAIT_UNTIL(c.objects().size() == 1);

  // Give user a box item
  User &user = s.getFirstUser();
  user.giveItem(&*s.items().begin());
  CHECK(c.waitForMessage(SV_INVENTORY));

  // Try to put item in object
  size_t serial = c.objects().begin()->first;
  c.sendMessage(CL_SWAP_ITEMS, makeArgs(0, 0, serial, 0));

  // Should be the alert that the object's inventory has changed
  CHECK(c.waitForMessage(SV_INVENTORY));
}

TEST_CASE("Client-side containers don't spontaneously clear", "[container]") {
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

TEST_CASE("Merchant can use same slot for ware and price") {
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
        WAIT_UNTIL(invSlot.first == diamond);
      }
    }
  }
}
