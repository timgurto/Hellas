#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

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

TEST_CASE("When one user approaches another, he finds out about him", "[.slow][culling][remote]"){
    // Given a server with a large map;
    TestServer s = TestServer::WithData("signpost");

    // And a client at (10, 10);
    TestClient c = TestClient::WithData("signpost");
    WAIT_UNTIL(s.users().size() == 1);

    // And a client at (1000, 10);
    s.changePlayerSpawn(Point(1000, 10));
    RemoteClient rc("-data testing/data/signpost");
    WAIT_UNTIL(s.users().size() == 2);
    REPEAT_FOR_MS(500);
    CHECK(c.otherUsers().size() == 0);

    // When the first moves within range of the second
    while (c->character().location().x < 900){
        c.sendMessage(CL_LOCATION, makeArgs(1000, 10));

        // Then he becomes aware of him
        if (c.otherUsers().size() == 1)
            break;
        SDL_Delay(5);
    }
    CHECK(c.otherUsers().size() == 1);
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
