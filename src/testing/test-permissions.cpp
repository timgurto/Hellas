#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Objects have no owner by default", "[ownership]"){
    // When a basic object is created
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", { 10, 10 });
    WAIT_UNTIL(s.entities().size() == 1);

    // Then that object has no owner
    Object &rock = s.getFirstObject();
    CHECK_FALSE(rock.permissions().hasOwner());
}

TEST_CASE("Constructing an object grants ownership", "[ownership]"){
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

TEST_CASE("Public-access objects", "[ownership]"){
    // Given a rock with no owner
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    s.addObject("rock", { 10, 10 });
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

TEST_CASE("The owner can access an owned object", "[ownership]"){
    // Given a rock owned by a user
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.addObject("rock", { 10, 10 }, user.name());
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

TEST_CASE("A non-owner cannot access an owned object", "[ownership]"){
    // Given a rock owned by Alice
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", { 10, 10 });
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

TEST_CASE("A city can own an object", "[city][ownership]"){
    // Given a rock, and a city named Athens
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", { 10, 10 });
    Object &rock = s.getFirstObject();

    // When its owner is set to Athens
    rock.permissions().setCityOwner("athens");

    // Then an 'owner()' check matches the city of Athens;
    CHECK(rock.permissions().isOwnedByCity("athens"));

    // And an 'owner()' check doesn't match a player named Athens
    CHECK_FALSE(rock.permissions().isOwnedByPlayer("athens"));
}

TEST_CASE("City ownership is persistent", "[city][ownership][persistence]"){
    // Given a rock owned by Athens
    {
        TestServer s1 = TestServer::WithData("basic_rock");
        s1.cities().createCity("athens");
        s1.addObject("rock", { 10, 10 });
        Object &rock = s1.getFirstObject();
        rock.permissions().setCityOwner("athens");
    }

    // When a new server starts up
    TestServer s2 = TestServer::WithDataAndKeepingOldData("basic_rock");

    // Then the rock is still owned by Athens
    Object &rock = s2.getFirstObject();
    CHECK(rock.permissions().isOwnedByCity("athens"));
}

TEST_CASE("City members can use city objects", "[city][ownership]"){
    // Given a rock owned by Athens;
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", { 10, 10 });
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

TEST_CASE("Non-members cannot use city objects", "[city][ownership]"){
    // Given a rock owned by Athens;
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    s.addObject("rock", { 10, 10 });
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

TEST_CASE("Non-existent cities can't own objects", "[city][ownership]"){
    // Given a rock, and a server with no cities
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", { 10, 10 });

    // When the rock's owner is set to a nonexistent city "Athens"
    Object &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");

    // Then the rock has no owner;
    CHECK_FALSE(rock.permissions().hasOwner());
}

TEST_CASE("New objects are added to owner index", "[ownership]"){
    // Given a server with rock objects
    TestServer s = TestServer::WithData("basic_rock");

    // When a rock is added, owned by Alice
    s.addObject("rock", {}, "alice");

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

TEST_CASE("A removed object is removed from the object-owner index", "[ownership]"){
    // Given a server
    TestServer s = TestServer::WithData("basic_rock");
    // And a rock object owned by Alice
    s.addObject("rock", {}, "alice");

    // When the object is removed
    Object &rock = s.getFirstObject();
    s.removeEntity(rock);

    // Then the object-owner index reports no objects belonging to Alice
    Permissions::Owner owner(Permissions::Owner::PLAYER, "alice");
    WAIT_UNTIL (s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 0);
}

TEST_CASE("New ownership is reflected in the object-owner index", "[ownership]"){
    // Given a server with rock objects
    TestServer s = TestServer::WithData("basic_rock");

    // When a rock is added, owned by Alice;
    s.addObject("rock", {}, "alice");

    // And the rock's ownership is changed to Bob;
    Object &rock = s.getFirstObject();
    rock.permissions().setPlayerOwner("bob");

    // The server's object-owner index has it under Bob's name, not Alice's
    Permissions::Owner ownerAlice(Permissions::Owner::PLAYER, "alice");
    WAIT_UNTIL(s.objectsByOwner().getObjectsWithSpecificOwner(ownerAlice).size() == 0);
    Permissions::Owner ownerBob(Permissions::Owner::PLAYER, "bob");
    CHECK(s.objectsByOwner().getObjectsWithSpecificOwner(ownerBob).size() == 1);
}

TEST_CASE("Objects can be granted to citizens", "[king][city][ownership][grant]") {
    // Given a Rock object type;
    auto s = TestServer::WithData("basic_rock");

    // And a city, Athens;
    s.cities().createCity("athens");

    // And its king, Alice;
    auto c = TestClient::WithUsernameAndData("alice", "basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    auto &alice = s.getFirstUser();
    s.cities().addPlayerToCity(alice, "athens");
    s->makePlayerAKing(alice);

    // And a rock, owned by Athens
    s.addObject("rock");
    auto &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");

    // When Alice tries to grant the rock to herself
    c.sendMessage(CL_GRANT, makeArgs(rock.serial(), "alice"));

    // Then the rock is owned by Alice
    WAIT_UNTIL(rock.permissions().isOwnedByPlayer("alice"));
}

TEST_CASE("Non kings can't grant objects", "[king][city][ownership][grant]") {
    // Given a Rock object type;
    auto s = TestServer::WithData("basic_rock");

    // And a city, Athens;
    s.cities().createCity("athens");

    // And its citizen, Alice;
    auto c = TestClient::WithUsernameAndData("alice", "basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    auto &alice = s.getFirstUser();
    s.cities().addPlayerToCity(alice, "athens");

    // And a rock, owned by Athens
    s.addObject("rock");
    auto &rock = s.getFirstObject();
    rock.permissions().setCityOwner("athens");

    // When Alice tries to grant the rock to herself
    c.sendMessage(CL_GRANT, makeArgs(rock.serial(), "alice"));

    // Then Alice receives an error message;
    c.waitForMessage(SV_NOT_A_KING);

    // And the rock is still owned by the city
    CHECK(rock.permissions().isOwnedByCity("athens"));
}

TEST_CASE("Unowned objects cannot be granted", "[city][ownership][grant]") {
    // Given a Rock object type;
    auto s = TestServer::WithData("basic_rock");

    // And a city, Athens;
    s.cities().createCity("athens");

    // And its king, Alice;
    auto c = TestClient::WithUsernameAndData("alice", "basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    auto &alice = s.getFirstUser();
    s.cities().addPlayerToCity(alice, "athens");
    s->makePlayerAKing(alice);
    
    // And an unowned rock
    s.addObject("rock");

    // When Alice tries to grant the rock to herself
    WAIT_UNTIL(s.users().size() == 1);
    auto &rock = s.getFirstObject();
    c.sendMessage(CL_GRANT, makeArgs(rock.serial(), "alice"));

    // Then Alice receives an error message;
    c.waitForMessage(SV_NO_PERMISSION);

    // And the rock is still unowned
    CHECK_FALSE(rock.permissions().hasOwner());
}

TEST_CASE("Only objects owned by your city can be granted", "[king][city][ownership][grant]") {
    // Given a Rock object type;
    auto s = TestServer::WithData("basic_rock");

    // And two cities, Athens and Sparts;
    s.cities().createCity("athens");
    s.cities().createCity("sparta");

    // And Alice, Athens' king;
    auto c = TestClient::WithUsernameAndData("alice", "basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    auto &alice = s.getFirstUser();
    s.cities().addPlayerToCity(alice, "athens");
    s->makePlayerAKing(alice);

    // And a rock, owned by Sparta
    s.addObject("rock");
    auto &rock = s.getFirstObject();
    rock.permissions().setCityOwner("sparta");

    // When Alice tries to grant the rock to herself
    WAIT_UNTIL(s.users().size() == 1);
    c.sendMessage(CL_GRANT, makeArgs(rock.serial(), "alice"));

    // Then Alice receives an error message;
    c.waitForMessage(SV_NO_PERMISSION);

    // And the rock is still owned by the city
    CHECK(rock.permissions().isOwnedByCity("sparta"));
}

TEST_CASE("New object permissions are propagated to clients", "[ownership]") {
    // Given a Rock object type;
    auto s = TestServer::WithData("basic_rock");
    auto c = TestClient::WithData("basic_rock");

    // And an unowned rock
    s.addObject("rock");
    WAIT_UNTIL(c.objects().size() == 1);

    // When the rock's owner is set to the user
    auto &rock = s.getFirstObject();
    rock.permissions().setPlayerOwner(c->username());

    // Then he finds out
    WAIT_UNTIL(c.getFirstObject().belongsToPlayer());
}

TEST_CASE("The first player to attack an object tags it"){
    // Given a client and server with rock objects;
    // And a rock object owned by Alice;
    // When the client attacks it
    // Then the client can loot it
}
