#include "TestClient.h"
#include "RemoteClient.h"
#include "Test.h"
#include "TestServer.h"

ONLY_TEST("Run a client in a separate process")
    TestServer s; s.run();
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 1);
    return true;
TEND

ONLY_TEST("Concurrent local and remote clients")
    TestServer s; s.run();
    TestClient c; c.run();
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 2);
    return true;
TEND

ONLY_TEST("Run CTI with custom username")
    TestServer s; s.run();
    TestClient alice("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);
    return alice->username() == "alice";
TEND

ONLY_TEST("Basic declaration of war")
    TestServer s; s.run();
    TestClient alice("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);

    alice.sendMessage(CL_DECLARE_WAR, "bob");
    WAIT_UNTIL_TIMEOUT(s.wars().isAtWar("alice", "bob"), 500);
    return true;
TEND

ONLY_TEST("No erroneous wars")
    TestServer s; s.run();
    return ! s.wars().isAtWar("alice", "bob");
TEND

ONLY_TEST("Wars are persistent")
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
