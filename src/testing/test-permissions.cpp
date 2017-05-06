#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

ONLY_TEST("Objects have no owner by default")
    // When a basic object is created
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL(s.objects().size() == 1);

    // Then that object has no owner
    Object &rock = s.getFirstObject();
    return ! rock.permissions().hasOwner();
TEND

ONLY_TEST("Constructing an object grants ownership")
    // Given a logged-in client
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    // When he constructs a wall
    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.objects().size() == 1);

    // Then he is the wall's owner
    Object &wall = s.getFirstObject();
    return
        wall.permissions().hasOwner() &&
        wall.permissions().isOwnedByPlayer(c->username());
TEND

ONLY_TEST("Public-access objects")
    // Given a rock with no owner
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // When a user attempts to gather it
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));

    // Then he gathers, receives a rock item, and the rock object disapears
    User &user = s.getFirstUser();
    WAIT_UNTIL (user.action() == User::Action::GATHER);
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION);
    WAIT_UNTIL_TIMEOUT(s.objects().empty(), 200);
    const Item &rockItem = s.getFirstItem();
    WAIT_UNTIL_TIMEOUT(user.inventory()[0].first == &rockItem, 200);
    return true;
TEND

ONLY_TEST("The owner can access an owned object")
    // Given a rock owned by a user
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.addObject("rock", Point(10, 10), &user);
    WAIT_UNTIL (c.objects().size() == 1);

    // When he attempts to gather it
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));

    // Then he gathers, receives a rock item, and the object disapears
    WAIT_UNTIL (user.action() == User::Action::GATHER);
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION);
    WAIT_UNTIL_TIMEOUT(s.objects().empty(), 200);
    const Item &rockItem = s.getFirstItem();
    WAIT_UNTIL_TIMEOUT(user.inventory()[0].first == &rockItem, 200);
    return true;
TEND

ONLY_TEST("A non-owner cannot access an owned object")
    // Given a rock owned by Alice
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));
    Object &rock = s.getFirstObject();
    rock.permissions().setPlayerOwner("alice");

    // When a different user attempts to gather it
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL (c.objects().size() == 1);
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));

    // Then the rock remains, and his inventory remains empty
    REPEAT_FOR_MS(500)
        if (s.objects().empty())
            return false;
    User &user = s.getFirstUser();
    if (user.inventory()[0].first != nullptr)
        return false;
    return true;
TEND

ONLY_TEST("A city can own an object")
    // Given a rock
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));
    Object &rock = s.getFirstObject();

    // When its owner is set to the city of Athens
    rock.permissions().setCityOwner("athens");

    // Then an 'owner()' check matches the city of Athens;
    if (! rock.permissions().isOwnedByCity("athens"))
        return false;

    // And an 'owner()' check doesn't match a player named Athens
    if (rock.permissions().isOwnedByPlayer("athens"))
        return false;

    return true;
TEND
