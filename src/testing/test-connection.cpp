#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Start and stop server")
    TestServer server;
    return true;
TEND

TEST("Run a client in a separate process")
    TestServer s;
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 1);
    return true;
TEND

TEST("Concurrent local and remote clients")
    TestServer s;
    TestClient c;
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 2);
    return true;
TEND

TEST("Run TestClient with custom username")
    TestServer s;
    TestClient alice = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);
    return alice->username() == "alice";
TEND

TEST("Removed users are removed from co-ord indices")
    TestServer s;
    {
        TestClient c;
    }
    WAIT_UNTIL(s.users().empty());
    return s.objectsByX().empty();
TEND
