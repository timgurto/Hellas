#include <cassert>

#include "RemoteClient.h"
#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"
#include "../client/ClientNPC.h"

TEST("Players can attack immediately")
    TestServer s = TestServer::WithData("ant");
    TestClient c = TestClient::WithData("ant");

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();

    // Add NPC
    s.addNPC("ant", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // Start attacking
    const auto objects = c.objects();
    const ClientObject *objP = objects.begin()->second;
    const ClientNPC &ant = dynamic_cast<const ClientNPC &>(*objP);
    size_t serial = ant.serial();
    c.sendMessage(CL_TARGET_ENTITY, makeArgs(serial));
    
    // NPC should be damaged very quickly
    REPEAT_FOR_MS(200)
        if (ant.health() < ant.npcType()->maxHealth())
            return true;
    return false;
TEND

TEST("Belligerents can target each other")
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    RemoteClient bob("-username bob");
    WAIT_UNTIL(s.users().size() == 2);
    User
        &uAlice = s.findUser("alice"),
        &uBob = s.findUser("bob");

    alice.sendMessage(CL_DECLARE_WAR, "bob");
    alice.sendMessage(CL_TARGET_PLAYER, "bob");
    WAIT_UNTIL(uAlice.target() == &uBob);

    return true;
TEND

TEST("Peaceful players can't target each other")
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    RemoteClient bob("-username bob");
    WAIT_UNTIL(s.users().size() == 2);
    User
        &uAlice = s.findUser("alice"),
        &uBob = s.findUser("bob");

    alice.sendMessage(CL_TARGET_PLAYER, "bob");
    REPEAT_FOR_MS(500) {
        if (uAlice.target() == &uBob)
            return false;
    }

    return true;
TEND

TEST("Belliegerents can fight")
    TestServer s;
    TestClient alice = TestClient::WithUsername("alicex");
    RemoteClient bob("-username bobx");
    WAIT_UNTIL(s.users().size() == 2);

    alice.sendMessage(CL_DECLARE_WAR, "bobx");

    User
        &uAlice = s.findUser("alicex"),
        &uBob = s.findUser("bobx");
    while (distance(uAlice.location(), uBob.location()) > Server::ACTION_DISTANCE)
        uAlice.updateLocation(uBob.location());

    WAIT_UNTIL(alice->isAtWarWith("bobx"));

    alice.sendMessage(CL_TARGET_PLAYER, "bobx");
    WAIT_UNTIL(uBob.health() < uBob.maxHealth());

    return true;
TEND

TEST("Peaceful players can't fight")
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    RemoteClient bob("-username bob");
    WAIT_UNTIL(s.users().size() == 2);

    User
        &uAlice = s.findUser("alice"),
        &uBob = s.findUser("bob");
    while (distance(uAlice.location(), uBob.location()) > Server::ACTION_DISTANCE)
        uAlice.updateLocation(uBob.location());

    alice.sendMessage(CL_TARGET_PLAYER, "bob");
    REPEAT_FOR_MS(500) {
        if (uBob.health() < uBob.maxHealth())
            return false;
    }

    return true;
TEND

TEST("Attack rate is respected")
    // Given a server, with a wolf NPC which hits for 1 damage every 100ms;
    TestServer s = TestServer::WithData("wolf");
    s.addNPC("wolf", Point(10, 20));

    // And a nearby user
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);
    const User &user = s.getFirstUser();
    health_t before = user.health();

    // When 1050ms elapse
    REPEAT_FOR_MS(1050);

    // Then the user has taken exactly 10 damage
    health_t after = user.health();
    return before - after == 10;
TEND

TEST("Belligerents can attack each other's objects")
    // Given a logged-in user;
    // And a vase object type with 1 health;
    TestServer s = TestServer::WithData("vase");
    TestClient c = TestClient::WithData("vase");

    // And a vase owned by Alice;
    s.addObject("vase", Point(10, 15), "alice");
    Object &vase = s.getFirstObject();
    assert(vase.health() == 1);

    // And that the user is at war with Alice
    const std::string &username = c.name();
    s.wars().declare(username, "alice");

    // When he targets the vase
    WAIT_UNTIL(s.users().size() == 1);
    c.sendMessage(CL_TARGET_ENTITY, makeArgs(vase.serial()));

    // Then the vase has 0 health
    WAIT_UNTIL(vase.health() == 0);

    return true;
TEND

ONLY_TEST("Players can target distant entities")
    // Given a server and client;
    TestServer s = TestServer::WithData("wolf");
    TestClient c = TestClient::WithData("wolf");

    // And a wolf NPC on the other side of the map
    s.addNPC("wolf", Point(200, 200));
    WAIT_UNTIL(s.users().size() == 1);
    const NPC &wolf = s.getFirstNPC();
    const User &user = s.getFirstUser();
    assert(distance(wolf.collisionRect(), user.collisionRect()) > Server::ACTION_DISTANCE);

    // When the client attempts to target the wolf
    WAIT_UNTIL(c.objects().size() == 1);
    ClientNPC &clientWolf = c.getFirstNPC();
    clientWolf.onRightClick(c.client());

    // Then his target is set to the wolf
    WAIT_UNTIL(user.target() == &wolf);
    return true;
TEND
