#include "RemoteClient.h"
#include "TestServer.h"
#include "TestClient.h"
#include "testing.h"
#include "../client/ClientNPC.h"

TEST_CASE("Players can attack immediately"){
    TestServer s = TestServer::WithData("ant");
    TestClient c = TestClient::WithData("ant");

    //Move user to middle
    s.waitForUsers(1);
    User &user = s.getFirstUser();

    // Add NPC
    s.addNPC("ant", { 10, 10 });
    WAIT_UNTIL (c.objects().size() == 1);

    // Start attacking
    const auto objects = c.objects();
    const ClientObject *objP = objects.begin()->second;
    const ClientNPC &ant = dynamic_cast<const ClientNPC &>(*objP);
    size_t serial = ant.serial();
    c.sendMessage(CL_TARGET_ENTITY, makeArgs(serial));
    
    // NPC should be damaged very quickly
    WAIT_UNTIL(ant.health() < ant.npcType()->maxHealth());
}

TEST_CASE("Belligerents can target each other", "[remote]"){
    TestServer s;
    TestClient alice = TestClient::WithUsername("Alice");
    RemoteClient bob("-username Bob");
    s.waitForUsers(2);
    User
        &uAlice = s.findUser("Alice"),
        &uBob = s.findUser("Bob");

    alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");
    alice.sendMessage(CL_TARGET_PLAYER, "Bob");
    WAIT_UNTIL(uAlice.target() == &uBob);
}

TEST_CASE("Peaceful players can't target each other", "[remote]"){
    TestServer s;
    TestClient alice = TestClient::WithUsername("Alice");
    RemoteClient bob("-username Bob");
    s.waitForUsers(2);
    User
        &uAlice = s.findUser("Alice"),
        &uBob = s.findUser("Bob");

    alice.sendMessage(CL_TARGET_PLAYER, "Bob");
    REPEAT_FOR_MS(500);
    CHECK_FALSE(uAlice.target() == &uBob);
}

TEST_CASE("Belliegerents can fight", "[remote]"){
    // Given a server, Alice, and Bob
    TestServer s;
    TestClient alice = TestClient::WithUsername("Alice");
    RemoteClient rcBob("-username Bob");
    s.waitForUsers(2);

    // And Alice is at war with Bob
    alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

    // When Alice moves within range of Bob
    User
        &uAlice = s.findUser("Alice"),
        &uBob = s.findUser("Bob");
    while (distance(uAlice.location(), uBob.location()) > Server::ACTION_DISTANCE)
        uAlice.updateLocation(uBob.location());

    // And Alice knows that the war has successfully been declared
    WAIT_UNTIL(alice.otherUsers().size() == 1);
    const auto &bob = alice.getFirstOtherUser();
    WAIT_UNTIL(alice->isAtWarWith(bob));

    // And Alice targets Bob
    alice.sendMessage(CL_TARGET_PLAYER, "Bob");

    // Then Bob loses health
    WAIT_UNTIL(uBob.health() < uBob.stats().maxHealth);
}

TEST_CASE("Peaceful players can't fight", "[remote]"){
    TestServer s;
    TestClient alice = TestClient::WithUsername("Alice");
    RemoteClient bob("-username Bob");
    s.waitForUsers(2);

    User
        &uAlice = s.findUser("Alice"),
        &uBob = s.findUser("Bob");
    while (distance(uAlice.location(), uBob.location()) > Server::ACTION_DISTANCE)
        uAlice.updateLocation(uBob.location());

    alice.sendMessage(CL_TARGET_PLAYER, "Bob");
    REPEAT_FOR_MS(500);

    CHECK (uBob.health() == uBob.stats().maxHealth);
}

TEST_CASE("Attack rate is respected", "[.flaky]"){
    // Given a server, with a wolf NPC which hits for 1 damage every 100ms;
    TestServer s = TestServer::WithData("wolf");
    s.addNPC("wolf", { 10, 20 });

    // And a nearby user
    TestClient c;
    s.waitForUsers(1);
    const User &user = s.getFirstUser();
    Hitpoints before = user.health();

    // When 1050ms elapse
    REPEAT_FOR_MS(1050);

    // Then the user has taken exactly 10 damage
    Hitpoints after = user.health();
    CHECK(after == before - 10);
}

TEST_CASE("Belligerents can attack each other's objects"){
    // Given a logged-in user;
    // And a vase object type with 1 health;
    TestServer s = TestServer::WithData("vase");
    TestClient c = TestClient::WithData("vase");

    // And a vase owned by Alice;
    s.addObject("vase", { 10, 15 }, "Alice");
    Object &vase = s.getFirstObject();
    REQUIRE(vase.health() == 1);

    // And that the user is at war with Alice
    const std::string &username = c.name();
    s.wars().declare(username, "Alice");

    // When he targets the vase
    s.waitForUsers(1);
    c.sendMessage(CL_TARGET_ENTITY, makeArgs(vase.serial()));

    // Then the vase has 0 health
    WAIT_UNTIL(vase.health() == 0);
}

TEST_CASE("Players can target distant entities"){
    // Given a server and client;
    TestServer s = TestServer::WithData("wolf");
    TestClient c = TestClient::WithData("wolf");

    // And a wolf NPC on the other side of the map
    s.addNPC("wolf", { 200, 200 });
    s.waitForUsers(1);
    const NPC &wolf = s.getFirstNPC();
    const User &user = s.getFirstUser();
    REQUIRE(distance(wolf.collisionRect(), user.collisionRect()) > Server::ACTION_DISTANCE);

    // When the client attempts to target the wolf
    WAIT_UNTIL(c.objects().size() == 1);
    ClientNPC &clientWolf = c.getFirstNPC();
    clientWolf.onRightClick(c.client());

    // Then his target is set to the wolf
    WAIT_UNTIL(user.target() == &wolf);
}

TEST_CASE("Clients receive nearby users' health values", "[remote]"){
    // Given a server and two clients, Alice and Bob;
    TestServer s;
    TestClient clientAlice = TestClient::WithUsername("Alice");
    RemoteClient clientBob = RemoteClient("-username Bob");

    // And Alice and Bob are at war
    s.wars().declare("Alice", "Bob");

    // When Alice is close to Bob;
    s.waitForUsers(2);
    const User
        &alice = s.findUser("Alice"),
        &bob = s.findUser("Bob");
    while (distance(alice.collisionRect(), bob.collisionRect()) >= Server::ACTION_DISTANCE){
        clientAlice.sendMessage(CL_LOCATION, makeArgs(bob.location().x, bob.location().y));
        SDL_Delay(5);
    }

    // And she attacks him
    clientAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

    // Then Alice sees that Bob is damaged
    WAIT_UNTIL(clientAlice.otherUsers().size() == 1);
    const Avatar &localBob = clientAlice.getFirstOtherUser();
    WAIT_UNTIL(localBob.health() < localBob.maxHealth());
}

TEST_CASE("A player dying doesn't crash the server"){
    // Given a server and client;
    TestServer s;
    TestClient c;
    s.waitForUsers(1);

    // When the user dies
    User &user = s.getFirstUser();
    user.reduceHealth(999999);

    // The server survives
    s.nop();
}
