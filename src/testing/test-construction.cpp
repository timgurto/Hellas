#include "TestServer.h"
#include "TestClient.h"
#include "testing.h"

TEST_CASE("Construction materials can be added"){
    // Given a server and client;
    // And a 'wall' object type that requires a brick for construction;
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    // And a brick item in the user's inventory
    WAIT_UNTIL (s.users().size() == 1);
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

TEST_CASE("Client knows about default constructions"){
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    CHECK(c.knowsConstruction("wall"));
}

TEST_CASE("New client doesn't know any locked constructions"){
    TestServer s = TestServer::WithData("secret_table");
    TestClient c = TestClient::WithData("secret_table");
    WAIT_UNTIL (s.users().size() == 1);

    CHECK_FALSE(c.knowsConstruction("table"));
}

TEST_CASE("Unique objects are unique"){
    TestServer s = TestServer::WithData("unique_throne");
    TestClient c = TestClient::WithData("unique_throne");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    WAIT_UNTIL (s.entities().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    bool isConstructionRejected = c.waitForMessage(SV_UNIQUE_OBJECT);

    CHECK(isConstructionRejected);
    CHECK(s.entities().size() == 1);
}

TEST_CASE("Constructing invalid object fails gracefully"){
    TestServer s;
    TestClient c;
    c.sendMessage(CL_CONSTRUCT, makeArgs("notARealObject", 10, 10));
}

TEST_CASE("Objects can be unbuildable"){
    TestServer s = TestServer::WithData("unbuildable_treehouse");
    TestClient c = TestClient::WithData("unbuildable_treehouse");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 10));
    REPEAT_FOR_MS(1200);

    CHECK(s.entities().size() == 0);
}

TEST_CASE("Clients can't see unbuildable constructions"){
    TestServer s = TestServer::WithData("unbuildable_treehouse");
    TestClient c = TestClient::WithData("unbuildable_treehouse");
    WAIT_UNTIL (s.users().size() == 1);
    
    CHECK_FALSE(c.knowsConstruction("treehouse"));
}

TEST_CASE("Objects without materials can't be built"){
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL (s.users().size() == 1);
    
    c.sendMessage(CL_CONSTRUCT, makeArgs("rock", 10, 15));
    REPEAT_FOR_MS(1200);
    CHECK(s.entities().size() == 0);
}

TEST_CASE("Construction tool requirements are enforced"){
    TestServer s = TestServer::WithData("computer");
    TestClient c = TestClient::WithData("computer");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("computer", 10, 10));
    CHECK(c.waitForMessage(SV_NEED_TOOLS));
    CHECK(s.entities().size() == 0);
}
