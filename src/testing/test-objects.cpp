#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Thin objects block movement") {
    // Given a server and client;
    auto s = TestServer::WithData("thin_wall");
    auto c = TestClient::WithData("thin_wall");

    // And a wall just above the user
    s.addObject("wall", { 10, 5 });

    // When the user tries to move up, through the wall
    s.waitForUsers(1);
    REPEAT_FOR_MS(500) {
        c.sendMessage(CL_LOCATION, makeArgs(10, 3));
        SDL_Delay(5);
    }

    // He fails
    auto &user = s.getFirstUser();
    CHECK(user.location().y > 4);
}

TEST_CASE("Dead objects don't block movement") {
    // Given a server and client;
    auto s = TestServer::WithData("thin_wall");
    auto c = TestClient::WithData("thin_wall");

    // And a wall just above the user;
    s.addObject("wall", { 10, 5 });

    // And that wall is dead
    s.getFirstObject().reduceHealth(1000000);

    // When the user tries to move up, through the wall
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    REPEAT_FOR_MS(3000) {
        c.sendMessage(CL_LOCATION, makeArgs(10, 3));

        if (user.location().y < 3.5)
            break;
    }
    // He succeeds
    CHECK(user.location().y < 3.5);;
}

TEST_CASE("Damaged objects can't be deconstructed") {
    // Given a server and client;
    // And a 'brick' object type with 2 health;
    TestServer s = TestServer::WithData("pickup_bricks");
    TestClient c = TestClient::WithData("pickup_bricks");

    // And a brick object exists with only 1 health
    s.addObject("brick", { 10, 15 });
    Object &brick = s.getFirstObject();
    brick.reduceHealth(1);
    REQUIRE(brick.health() == 1);

    // When the user tries to deconstruct the brick
    c.sendMessage(CL_DECONSTRUCT, makeArgs(brick.serial()));

    // Then the object still exists
    REPEAT_FOR_MS(100);
    CHECK_FALSE(s.entities().empty());
}

TEST_CASE("Out-of-range objects are forgotten", "[.slow][culling][only]") {
    // Given a server and client with signpost objects;
    TestServer s = TestServer::WithData("signpost");
    TestClient c = TestClient::WithData("signpost");

    // And a signpost near the user spawn
    s.addObject("signpost", { 10, 15 });

    // And the client is aware of it
    s.waitForUsers(1);
    WAIT_UNTIL(c.objects().size() == 1);

    // When the client moves out of range of the signpost
    while (c->character().location().x < 1000) {
        c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

        // Then he is no longer aware of it
        if (c.objects().size() == 0)
            break;
        SDL_Delay(5);
    }
    CHECK(c.objects().size() == 0);
}
