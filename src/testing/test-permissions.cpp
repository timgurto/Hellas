#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Objects have no owner by default")
    // When a basic object is created
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL(s.entities().size() == 1);

    // Then that object has no owner
    Object &rock = s.getFirstObject();
    return ! rock.permissions().hasOwner();
TEND

TEST("Constructing an object grants ownership")
    // Given a logged-in client
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    // When he constructs a wall
    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.entities().size() == 1);

    // Then he is the wall's owner
    Object &wall = s.getFirstObject();
    return
        wall.permissions().hasOwner() &&
        wall.permissions().isOwnedByPlayer(c->username());
TEND

TEST("Public-access objects")
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
    WAIT_UNTIL_TIMEOUT(s.entities().empty(), 200);
    const Item &rockItem = s.getFirstItem();
    WAIT_UNTIL_TIMEOUT(user.inventory()[0].first == &rockItem, 200);
    return true;
TEND

TEST("The owner can access an owned object")
    // Given a rock owned by a user
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.addObject("rock", Point(10, 10), user.name());
    WAIT_UNTIL (c.objects().size() == 1);

    // When he attempts to gather it
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));

    // Then he gathers, receives a rock item, and the object disapears
    WAIT_UNTIL (user.action() == User::Action::GATHER);
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION);
    WAIT_UNTIL_TIMEOUT(s.entities().empty(), 200);
    const Item &rockItem = s.getFirstItem();
    WAIT_UNTIL_TIMEOUT(user.inventory()[0].first == &rockItem, 200);
    return true;
TEND

TEST("A non-owner cannot access an owned object")
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
        if (s.entities().empty())
            return false;
    User &user = s.getFirstUser();
    if (user.inventory()[0].first != nullptr)
        return false;
    return true;
TEND

TEST("A city can own an object")
    // Given a rock, and a city named Athens
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", Point(10, 10));
    Object &rock = s.getFirstObject();

    // When its owner is set to Athens
    rock.permissions().setCityOwner("athens");

    // Then an 'owner()' check matches the city of Athens;
    if (! rock.permissions().isOwnedByCity("athens"))
        return false;

    // And an 'owner()' check doesn't match a player named Athens
    if (rock.permissions().isOwnedByPlayer("athens"))
        return false;

    return true;
TEND

TEST("City ownership is persistent")
    // Given a rock owned by Athens
    {
        TestServer s1 = TestServer::WithData("basic_rock");
        s1.cities().createCity("athens");
        s1.addObject("rock", Point(10, 10));
        Object &rock = s1.getFirstObject();
        rock.permissions().setCityOwner("athens");
    }

    // When a new server starts up
    TestServer s2 = TestServer::WithDataAndKeepingOldData("basic_rock");

    // Then the rock is still owned by Athens
    Object &rock = s2.getFirstObject();
    return rock.permissions().isOwnedByCity("athens");
TEND

TEST("City members can use city objects")
    // Given a rock owned by Athens;
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", Point(10, 10));
    Object &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");
    // And a client, who is a member of Athens
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.cities().addPlayerToCity(user, "athens");

    // When he attempts to gather the rock
    WAIT_UNTIL (c.objects().size() == 1);
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));

    // Then he gathers;
    WAIT_UNTIL (user.action() == User::Action::GATHER);
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION);
    // And he receives a Rock item;
    const Item &rockItem = s.getFirstItem();
    WAIT_UNTIL_TIMEOUT(user.inventory()[0].first == &rockItem, 200);
    // And the Rock object disappears
    WAIT_UNTIL_TIMEOUT(s.entities().empty(), 200);
    return true;
TEND

TEST("Non-members cannot use city objects")
    // Given a rock owned by Athens;
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", Point(10, 10));
    Object &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");
    // And a client, not a member of any city
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);

    // When he attempts to gather the rock
    WAIT_UNTIL (c.objects().size() == 1);
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));

    // Then the rock remains;
    REPEAT_FOR_MS(500)
        if (s.entities().empty())
            return false;
    // And his inventory remains empty
    User &user = s.getFirstUser();
    if (user.inventory()[0].first != nullptr)
        return false;
    return true;
TEND

TEST("Non-existent cities can't own objects")
    // Given a rock, and a server with no cities
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));

    // When the rock's owner is set to a nonexistent city "Athens"
    Object &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");

    // Then the rock has no owner;
    return ! rock.permissions().hasOwner();
TEND
