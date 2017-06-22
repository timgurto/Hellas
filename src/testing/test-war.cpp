#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Basic declaration of war"){
    // Given Alice is logged in
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice sends a CL_DECLARE_WAR message
    alice.sendMessage(CL_DECLARE_WAR, "bob");

    // Then Alice and Bob go to war
    WAIT_UNTIL(s.wars().isAtWar("alice", "bob"));
}

TEST_CASE("No erroneous wars"){
    // When a clean server is started
    TestServer s;
    
    // Then Alice and Bob are not at war
    CHECK_FALSE(s.wars().isAtWar("alice", "bob"));
}

TEST_CASE("Wars are persistent"){
    // Given Alice and Bob are at war, and there is no server running
    {
        TestServer server1;
        server1.wars().declare("alice", "bob");
    }

    // When a server begins that keeps persistent data
    TestServer server2 = TestServer::KeepingOldData();

    // Then Alice and Bob are still at war
    CHECK(server2.wars().isAtWar("alice", "bob"));
}

TEST_CASE("Clients are alerted of new wars"){
    // Given Alice is logged in
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice declares war on Bob
    alice.sendMessage(CL_DECLARE_WAR, "bob");

    // Then Alice is alerted to the new war
    WAIT_UNTIL(alice->isAtWarWith("bob"));
}

TEST_CASE("Clients are told of existing wars on login"){
    // Given Alice and Bob are at war
    TestServer s;
    s.wars().declare("alice", "bob");

    // When Alice logs in
    TestClient alice = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // Then she is told about the war
    WAIT_UNTIL(alice->isAtWarWith("bob"));
}

TEST_CASE("Wars cannot be redeclared"){
    // Given Alice and Bob are at war, and Alice is logged in
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    s.wars().declare("alice", "bob");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice declares war on Bob
    alice.sendMessage(CL_DECLARE_WAR, "bob");

    // Then she receives an SV_ALREADY_AT_WAR error message
    CHECK(alice.waitForMessage(SV_ALREADY_AT_WAR));
}
