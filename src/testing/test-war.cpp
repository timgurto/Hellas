#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Basic declaration of war")
    // Given Alice is logged in
    TestServer s;
    TestClient alice = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice sends a CL_DECLARE_WAR message
    alice.sendMessage(CL_DECLARE_WAR, "bob");

    // Then Alice and Bob go to war
    WAIT_UNTIL(s.wars().isAtWar("alice", "bob"));
    return true;
TEND

TEST("No erroneous wars")
    // When a clean server is started
    TestServer s;
    
    // Then Alice and Bob are not at war
    return ! s.wars().isAtWar("alice", "bob");
TEND

TEST("Wars are persistent")
    // Given Alice and Bob are at war, and there is no server running
    {
        TestServer server1;
        server1.wars().declare("alice", "bob");
    }

    // When a server begins that keeps persistent data
    TestServer server2 = TestServer::KeepOldData();

    // Then Alice and Bob are still at war
    return server2.wars().isAtWar("alice", "bob");
TEND

TEST("Clients are alerted of new wars")
    // Given Alice is logged in
    TestServer s;
    TestClient alice = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice declares war on Bob
    alice.sendMessage(CL_DECLARE_WAR, "bob");

    // Then Alice is alerted to the new war
    WAIT_UNTIL(alice->isAtWarWith("bob"));
    return true;
TEND

TEST("Clients are told of existing wars on login")
    // Given Alice and Bob are at war
    TestServer s;
    s.wars().declare("alice", "bob");

    // When Alice logs in
    TestClient alice = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // Then she is told about the war
    WAIT_UNTIL(alice->isAtWarWith("bob"));
    return true;
TEND

TEST("Wars cannot be redeclared")
    // Given Alice and Bob are at war, and Alice is logged in
    TestServer s;
    TestClient alice = TestClient::Username("alice");
    s.wars().declare("alice", "bob");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice declares war on Bob
    alice.sendMessage(CL_DECLARE_WAR, "bob");

    // Then she receives an SV_ALREADY_AT_WAR error message
    return alice.waitForMessage(SV_ALREADY_AT_WAR);
TEND
