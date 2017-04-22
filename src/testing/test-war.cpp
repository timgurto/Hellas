#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Basic declaration of war")
    TestServer s; s.run();
    TestClient alice = TestClient::Username("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);

    alice.sendMessage(CL_DECLARE_WAR, "bob");
    WAIT_UNTIL(s.wars().isAtWar("alice", "bob"));
    return true;
TEND

TEST("No erroneous wars")
    TestServer s; s.run();
    return ! s.wars().isAtWar("alice", "bob");
TEND

TEST("Wars are persistent")
    {
        TestServer server1;
        server1.run();
        server1.wars().declare("alice", "bob");
    }
    TestServer server2 = TestServer::KeepOldData();
    server2.run();
    bool result = server2.wars().isAtWar("alice", "bob");
    return result;
TEND

TEST("Clients are alerted of new wars")
    TestServer s;
    s.run();
    s.wars().declare("alice", "bob");

    TestClient alice = TestClient::Username("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);

    alice.sendMessage(CL_DECLARE_WAR, "bob");
    WAIT_UNTIL(alice->isAtWarWith("bob"));
    return true;
TEND
