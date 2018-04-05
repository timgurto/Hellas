#include "TestServer.h"
#include "TestClient.h"
#include "testing.h"

TEST_CASE("Construction materials can be added", "[construction]"){
    // Given a server and client;
    // And a 'wall' object type that requires a brick for construction;
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    // And a brick item in the user's inventory
    s.waitForUsers(1);
    User &user = s.getFirstUser();
    user.giveItem(&*s.items().begin());
    WAIT_UNTIL(c.inventory()[0].first != nullptr);

    // When the user places a construction site;
    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.entities().size() == 1);

    // And gives it his brick
    const Object &wall = s.getFirstObject();
    c.sendMessage(CL_SWAP_ITEMS, makeArgs(Server::INVENTORY, 0, wall.serial(), 0));

    // Then construction has finished
    WAIT_UNTIL(! wall.isBeingBuilt());
}

TEST_CASE("Client knows about default constructions", "[construction]"){
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    s.waitForUsers(1);

    CHECK(c.knowsConstruction("wall"));
}

TEST_CASE("New client doesn't know any locked constructions", "[construction]"){
    TestServer s = TestServer::WithData("secret_table");
    TestClient c = TestClient::WithData("secret_table");
    s.waitForUsers(1);

    CHECK_FALSE(c.knowsConstruction("table"));
}

TEST_CASE("Unique objects are unique"){
    TestServer s = TestServer::WithData("unique_throne");
    TestClient c = TestClient::WithData("unique_throne");
    s.waitForUsers(1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    WAIT_UNTIL (s.entities().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    bool isConstructionRejected = c.waitForMessage(WARNING_UNIQUE_OBJECT);

    CHECK(isConstructionRejected);
    CHECK(s.entities().size() == 1);
}

TEST_CASE("Constructing invalid object fails gracefully", "[construction]"){
    TestServer s;
    TestClient c;
    c.sendMessage(CL_CONSTRUCT, makeArgs("notARealObject", 10, 10));
}

TEST_CASE("Objects can be unbuildable"){
    TestServer s = TestServer::WithData("unbuildable_treehouse");
    TestClient c = TestClient::WithData("unbuildable_treehouse");
    s.waitForUsers(1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 10));
    REPEAT_FOR_MS(1200);

    CHECK(s.entities().size() == 0);
}

TEST_CASE("Clients can't see unbuildable constructions", "[construction]"){
    TestServer s = TestServer::WithData("unbuildable_treehouse");
    TestClient c = TestClient::WithData("unbuildable_treehouse");
    s.waitForUsers(1);
    
    CHECK_FALSE(c.knowsConstruction("treehouse"));
}

TEST_CASE("Objects without materials can't be built", "[construction]"){
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    s.waitForUsers(1);
    
    c.sendMessage(CL_CONSTRUCT, makeArgs("rock", 10, 15));
    REPEAT_FOR_MS(1200);
    CHECK(s.entities().size() == 0);
}

TEST_CASE("Construction tool requirements are enforced", "[construction]"){
    TestServer s = TestServer::WithData("computer");
    TestClient c = TestClient::WithData("computer");
    s.waitForUsers(1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("computer", 10, 10));
    CHECK(c.waitForMessage(WARNING_NEED_TOOLS));
    CHECK(s.entities().size() == 0);
}

TEST_CASE("Construction progress is persistent", "[persistence][construction]") {
    // Given a new brick wall object with no materials added, owned by Alice
    {
        auto s = TestServer::WithData("brick_wall");
        s.addObject("wall", { 10, 10 }, "Alice");

        // And Alice has a brick
        auto c = TestClient::WithUsernameAndData("Alice", "brick_wall");
        s.waitForUsers(1);
        auto &alice = s.getFirstUser();
        const auto *brick = &s.getFirstItem();
        alice.giveItem(brick);

        // When Alice adds a brick to the construction site
        const auto &wall = s.getFirstObject();
        c.sendMessage(CL_SWAP_ITEMS, makeArgs(Server::INVENTORY, 0, wall.serial(), 0));

        // And the construction finishes
        WAIT_UNTIL(!wall.isBeingBuilt());

        // And the server restarts
    }
    {
        auto s = TestServer::WithDataAndKeepingOldData("brick_wall");

        // Then the wall is still complete
        REPEAT_FOR_MS(100);
        WAIT_UNTIL(s.entities().size() == 1);
        const auto &wall = s.getFirstObject();
        CHECK(!wall.isBeingBuilt());
    }
}

TEST_CASE("A construction material can 'return' an item", "[construction]") {
    // Given a server and client;
    // And a 'fire' object type that requires matches;
    // And matches return a matchbox when used for construction
    auto s = TestServer::WithData("matches");
    auto c = TestClient::WithData("matches");
    // And matches are in the user's inventory
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    auto &matches = *s.items().find(ServerItem{ "matches" });
    user.giveItem(&matches);
    WAIT_UNTIL(c.inventory()[0].first != nullptr);

    // When the user places a fire construction site;
    c.sendMessage(CL_CONSTRUCT, makeArgs("fire", 10, 10));
    WAIT_UNTIL(s.entities().size() == 1);

    // And gives it his matches
    const Object &fire = s.getFirstObject();
    c.sendMessage(CL_SWAP_ITEMS, makeArgs(Server::INVENTORY, 0, fire.serial(), 0));

    // Then he receives a matchbox
    const auto &pItem = user.inventory()[0].first;
    WAIT_UNTIL(pItem != nullptr && pItem->id() == "matchbox");
}
