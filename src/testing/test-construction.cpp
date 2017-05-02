#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"

TEST("Construction materials can be added")
    TestServer s = TestServer::Data("brick_wall");
    TestClient c = TestClient::Data("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.objects().size() == 1);

    User &user = const_cast<User &>(*s.users().begin());
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
