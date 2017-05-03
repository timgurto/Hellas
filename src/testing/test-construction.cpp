#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"

TEST("Construction materials can be added")
    TestServer s = TestServer::Data("brick_wall");
    TestClient c = TestClient::Data("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.objects().size() == 1);

    User &user = s.getFirstUser();
    user.giveItem(&*s.items().begin());
    if (!c.waitForMessage(SV_INVENTORY))
        return false;

    const Object &wall = **s.objects().begin();
    c.sendMessage(CL_SWAP_ITEMS, makeArgs(Server::INVENTORY, 0, wall.serial(), 0));
    WAIT_UNTIL(! wall.isBeingBuilt());

    return true;
TEND

TEST("Client knows about default constructions")
    TestServer s = TestServer::Data("brick_wall");
    TestClient c = TestClient::Data("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    return c.knowsConstruction("wall");
TEND

TEST("New client doesn't know any locked constructions")
    TestServer s = TestServer::Data("secret_table");
    TestClient c = TestClient::Data("secret_table");
    WAIT_UNTIL (s.users().size() == 1);

    return ! c.knowsConstruction("table");
TEND

TEST("Unique objects are unique")
    TestServer s = TestServer::Data("unique_throne");
    TestClient c = TestClient::Data("unique_throne");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    WAIT_UNTIL (s.objects().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
    bool isConstructionRejected = c.waitForMessage(SV_UNIQUE_OBJECT);

    if (! isConstructionRejected)
        return false;
    if (s.objects().size() > 1)
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
    TestServer s = TestServer::Data("unbuildable_treehouse");
    TestClient c = TestClient::Data("unbuildable_treehouse");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 10));

    REPEAT_FOR_MS(1200)
        if (s.objects().size() == 1)
            return false;
    return true;
TEND

TEST("Clients can't see unbuildable constructions")
    TestServer s = TestServer::Data("unbuildable_treehouse");
    TestClient c = TestClient::Data("unbuildable_treehouse");
    WAIT_UNTIL (s.users().size() == 1);
    
    return ! c.knowsConstruction("treehouse");
TEND

TEST("Objects without materials can't be built")
    TestServer s = TestServer::Data("basic_rock");
    TestClient c = TestClient::Data("basic_rock");
    WAIT_UNTIL (s.users().size() == 1);
    
    c.sendMessage(CL_CONSTRUCT, makeArgs("rock", 10, 15));
    REPEAT_FOR_MS(1200)
        if (s.objects().size() == 1)
            return false;
    return true;
TEND

TEST("Construction tool requirements are enforced")
    TestServer s = TestServer::Data("computer");
    TestClient c = TestClient::Data("computer");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("computer", 10, 10));
    bool isConstructionRejected = c.waitForMessage(SV_NEED_TOOLS);

    if (! isConstructionRejected)
        return false;
    if (s.objects().size() > 1)
        return false;

    return true;
TEND
