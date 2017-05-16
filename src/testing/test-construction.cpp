#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"

TEST("Construction materials can be added")
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

    return true;
TEND

TEST("Client knows about default constructions")
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    return c.knowsConstruction("wall");
TEND

TEST("New client doesn't know any locked constructions")
    TestServer s = TestServer::WithData("secret_table");
    TestClient c = TestClient::WithData("secret_table");
    WAIT_UNTIL (s.users().size() == 1);

    return ! c.knowsConstruction("table");
TEND

TEST("Unique objects are unique")
    TestServer s = TestServer::WithData("unique_throne");
    TestClient c = TestClient::WithData("unique_throne");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    WAIT_UNTIL (s.entities().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    bool isConstructionRejected = c.waitForMessage(SV_UNIQUE_OBJECT);

    if (! isConstructionRejected)
        return false;
    if (s.entities().size() > 1)
        return false;

    return true;
TEND

TEST("Constructing invalid object fails gracefully")
    TestServer s;
    TestClient c;
    c.sendMessage(CL_CONSTRUCT, makeArgs("notARealObject", 10, 10));
    return true;
TEND

TEST("Objects can be unbuildable")
    TestServer s = TestServer::WithData("unbuildable_treehouse");
    TestClient c = TestClient::WithData("unbuildable_treehouse");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 10));

    REPEAT_FOR_MS(1200)
        if (s.entities().size() == 1)
            return false;
    return true;
TEND

TEST("Clients can't see unbuildable constructions")
    TestServer s = TestServer::WithData("unbuildable_treehouse");
    TestClient c = TestClient::WithData("unbuildable_treehouse");
    WAIT_UNTIL (s.users().size() == 1);
    
    return ! c.knowsConstruction("treehouse");
TEND

TEST("Objects without materials can't be built")
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL (s.users().size() == 1);
    
    c.sendMessage(CL_CONSTRUCT, makeArgs("rock", 10, 15));
    REPEAT_FOR_MS(1200)
        if (s.entities().size() == 1)
            return false;
    return true;
TEND

TEST("Construction tool requirements are enforced")
    TestServer s = TestServer::WithData("computer");
    TestClient c = TestClient::WithData("computer");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("computer", 10, 10));
    bool isConstructionRejected = c.waitForMessage(SV_NEED_TOOLS);

    if (! isConstructionRejected)
        return false;
    if (s.entities().size() > 1)
        return false;

    return true;
TEND
