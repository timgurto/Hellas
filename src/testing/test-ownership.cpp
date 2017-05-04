#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

ONLY_TEST("Objects have no owner by default")
    // When a basic object is created
    TestServer s = TestServer::Data("basic_rock");
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL(s.objects().size() == 1);

    // That object has no owner
    Object &rock = s.getFirstObject();
    return rock.hasOwner() == false;
TEND

ONLY_TEST("Constructing an object grants ownership")
    // Given a logged-in client
    TestServer s = TestServer::Data("brick_wall");
    TestClient c = TestClient::Data("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    // When he constructs a wall
    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.objects().size() == 1);

    // He is the wall's owner
    Object &wall = s.getFirstObject();
    return wall.hasOwner();
    /*&& wall.ownerName() == c->username()*/
TEND
