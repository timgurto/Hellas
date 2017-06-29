#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Objects have no owner by default"){
    // When a basic object is created
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL(s.entities().size() == 1);

    // Then that object has no owner
    Object &rock = s.getFirstObject();
    CHECK_FALSE(rock.permissions().hasOwner());
}

TEST_CASE("Constructing an object grants ownership"){
    // Given a logged-in client
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    // When he constructs a wall
    c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
    WAIT_UNTIL (s.entities().size() == 1);

    // Then he is the wall's owner
    Object &wall = s.getFirstObject();
    CHECK(wall.permissions().hasOwner());
    CHECK(wall.permissions().isOwnedByPlayer(c->username()));
}

TEST_CASE("Public-access objects"){
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
}

TEST_CASE("The owner can access an owned object"){
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
}

TEST_CASE("A non-owner cannot access an owned object"){
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
    REPEAT_FOR_MS(500);

    // Then the rock remains, and his inventory remains empty
    CHECK_FALSE(s.entities().empty());
    User &user = s.getFirstUser();
    CHECK(user.inventory()[0].first == nullptr);
}

TEST_CASE("A city can own an object"){
    // Given a rock, and a city named Athens
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", Point(10, 10));
    Object &rock = s.getFirstObject();

    // When its owner is set to Athens
    rock.permissions().setCityOwner("athens");

    // Then an 'owner()' check matches the city of Athens;
    CHECK(rock.permissions().isOwnedByCity("athens"));

    // And an 'owner()' check doesn't match a player named Athens
    CHECK_FALSE(rock.permissions().isOwnedByPlayer("athens"));
}

TEST_CASE("City ownership is persistent"){
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
    CHECK(rock.permissions().isOwnedByCity("athens"));
}

TEST_CASE("City members can use city objects"){
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
}

TEST_CASE("Non-members cannot use city objects"){
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
    REPEAT_FOR_MS(500);

    // Then the rock remains;
    CHECK_FALSE(s.entities().empty());

    // And his inventory remains empty
    User &user = s.getFirstUser();
    CHECK(user.inventory()[0].first == nullptr);
}

TEST_CASE("Non-existent cities can't own objects"){
    // Given a rock, and a server with no cities
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", Point(10, 10));

    // When the rock's owner is set to a nonexistent city "Athens"
    Object &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");

    // Then the rock has no owner;
    CHECK_FALSE(rock.permissions().hasOwner());
}

TEST_CASE("On login, players are told about their distant objects", "[.flaky][culling]"){
    // Given an object at (10000,10000) owned by Alice
    TestServer s = TestServer::WithData("signpost");
    s.addObject("signpost", Point(10000, 10000), "alice");

    // When Alice logs in
    TestClient c = TestClient::WithUsernameAndData("alice", "signpost");
    WAIT_UNTIL_TIMEOUT(s.users().size() == 1, 10000);

    // Alice knows about the object
    REPEAT_FOR_MS(500);
    CHECK(c.objects().size() == 1);
}

TEST_CASE("On login, players are not told about others' distant objects", "[.flaky][culling]"){
    // Given an object at (10000,10000) owned by Alice
    TestServer s = TestServer::WithData("signpost");
    s.addObject("signpost", Point(10000, 10000), "bob");

    // When Alice logs in
    TestClient c = TestClient::WithUsernameAndData("alice", "signpost");
    WAIT_UNTIL_TIMEOUT(s.users().size() == 1, 10000);

    // Alice does not know about the object
    REPEAT_FOR_MS(500);
    CHECK(c.objects().empty());
}

TEST_CASE("New objects are added to owner index"){
    // Given a server with rock objects
    TestServer s = TestServer::WithData("basic_rock");

    // When a rock is added, owned by Alice
    s.addObject("rock", Point(), "alice");

    // The server's object-owner index knows about it
    Permissions::Owner owner(Permissions::Owner::PLAYER, "alice");
    WAIT_UNTIL(s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 1);
}

TEST_CASE("The object-owner index is initially empty"){
    // Given an empty server
    TestServer s;

    // Then the object-owner index reports no objects belonging to Alice
    Permissions::Owner owner(Permissions::Owner::PLAYER, "alice");
    CHECK(s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 0);
}

TEST_CASE("A removed object is removed from the object-owner index"){
    // Given a server
    TestServer s = TestServer::WithData("basic_rock");
    // And a rock object owned by Alice
    s.addObject("rock", Point(), "alice");

    // When the object is removed
    Object &rock = s.getFirstObject();
    s.removeEntity(rock);

    // Then the object-owner index reports no objects belonging to Alice
    Permissions::Owner owner(Permissions::Owner::PLAYER, "alice");
    WAIT_UNTIL (s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 0);
}

TEST_CASE("New ownership is reflected in the object-owner index"){
    // Given a server with rock objects
    TestServer s = TestServer::WithData("basic_rock");

    // When a rock is added, owned by Alice;
    s.addObject("rock", Point(), "alice");

    // And the rock's ownership is changed to Bob;
    Object &rock = s.getFirstObject();
    rock.permissions().setPlayerOwner("bob");

    // The server's object-owner index has it under Bob's name, not Alice's
    Permissions::Owner ownerAlice(Permissions::Owner::PLAYER, "alice");
    WAIT_UNTIL(s.objectsByOwner().getObjectsWithSpecificOwner(ownerAlice).size() == 0);
    Permissions::Owner ownerBob(Permissions::Owner::PLAYER, "bob");
    CHECK(s.objectsByOwner().getObjectsWithSpecificOwner(ownerBob).size() == 1);
}

TEST_CASE("When a player moves away from his object, he is still aware of it", "[.slow][culling]"){
    // Given a server with signpost objects;
    TestServer s = TestServer::WithData("signpost");

    // And a signpost near the user spawn point that belongs to Alice;
    s.addObject("signpost", Point(10, 15), "alice");

    // And Alice is logged in
    TestClient c = TestClient::WithUsernameAndData("alice", "signpost");
    WAIT_UNTIL(s.users().size() == 1);
    WAIT_UNTIL(c.objects().size() == 1);

    // When Alice moves out of range of the signpost
    while (c->character().location().x < 1000){
        c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

        // Then she is still aware of it
        if (c.objects().size() == 0)
            break;
        SDL_Delay(5);
    }
    CHECK(c.objects().size() == 1);
}

TEST_CASE("When a player moves away from his city's object, he is still aware of it",
          "[.slow][culling]"){
    // Given a server with signpost objects;
    TestServer s = TestServer::WithData("signpost");

    // And a city named Athens
    s.cities().createCity("athens");

    // And a signpost near the user spawn point that belongs to Athens;
    s.addObject("signpost", Point(10, 15));
    Object &signpost = s.getFirstObject();
    signpost.permissions().setCityOwner("athens");

    // And Alice is logged in
    TestClient c = TestClient::WithUsernameAndData("alice", "signpost");

    // And Alice is a member of Athens
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.cities().addPlayerToCity(user, "athens");

    // When Alice moves out of range of the signpost
    WAIT_UNTIL(c.objects().size() == 1);
    while (c->character().location().x < 1000){
        c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

        // Then she is still aware of it
        if (c.objects().size() == 0)
            break;
        SDL_Delay(5);
    }
    CHECK(c.objects().size() == 1);
}
