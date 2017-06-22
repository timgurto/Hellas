#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Start and stop server")
    TestServer server;
    return true;
TEND

TEST("Run a client in a separate process")
    // Given a server
    TestServer s;

    // When a RemoteClient is created
    RemoteClient alice("-username alice");

    // Then it successfully logs into the server
    WAIT_UNTIL(s.users().size() == 1);
    return true;
TEND

TEST("Concurrent local and remote clients")
    // Given a server
    TestServer s;

    // When a TestClient and RemoteClient are both created
    TestClient c;
    RemoteClient alice("-username alice");

    // Then both successfully log into the server
    WAIT_UNTIL(s.users().size() == 2);
    return true;
TEND

TEST("Run TestClient with custom username")
    // Given a server
    TestServer s;

    // When a TestClient is created with a custom username
    TestClient alice = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // Then the client logs in with that username
    return alice->username() == "alice";
TEND

TEST("Removed users are removed from co-ord indices")
    //Given a server
    TestServer s;

    // When a client connects and disconnects
    {
        TestClient c;
    }
    WAIT_UNTIL(s.users().empty());

    // Then that user is not represented in the x-indexed objects list
    return s.entitiesByX().empty();
TEND

SLOW_TEST("Server remains functional with unresponsive client")
    // Given a server and client
    TestServer s;
    {
        TestClient badClient;
        WAIT_UNTIL(s.users().size() == 1);

        // When the client freezes and becomes unresponsive;
        badClient.freeze();

        // And times out on the server
       WAIT_UNTIL_TIMEOUT(s.users().size() == 0, 15000);
    }

    // Then the server can still accept connections
    TestClient goodClient;
    WAIT_UNTIL(s.users().size() == 1);

    return true;
TEND

QUARANTINED_TEST("Map with extra row doesn't crash client")
    TestServer s;
    TestClient c = TestClient::WithData("abort");
    REPEAT_FOR_MS(1000)
        ;
TEND

TEST_CASE("New servers clear old user data"){
    {
        TestServer s;
        TestClient c = TestClient::WithUsername("alice");
        WAIT_UNTIL(s.users().size() == 1);
        User &alice = s.getFirstUser();
        CHECK(alice.health() == alice.maxHealth());
        alice.reduceHealth(1);
        CHECK(alice.health() < alice.maxHealth());
    }
    {
        TestServer s;
        TestClient c = TestClient::WithUsername("alice");
        WAIT_UNTIL(s.users().size() == 1);
        User &alice = s.getFirstUser();
        CHECK(alice.health() == alice.maxHealth());
    }
}
