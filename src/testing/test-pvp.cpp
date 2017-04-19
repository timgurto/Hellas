#include "ClientTestInterface.h"
#include "RemoteClient.h"
#include "Test.h"
#include "ServerTestInterface.h"

ONLY_TEST("Run a client in a separate process")
    ServerTestInterface s; s.run();
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 1);
    return true;
TEND

ONLY_TEST("Concurrent local and remote clients")
    ServerTestInterface s; s.run();
    ClientTestInterface c; c.run();
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 2);
    return true;
TEND

ONLY_TEST("Run CTI with custom username")
    ServerTestInterface s; s.run();
    ClientTestInterface alice("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);
    return alice->username() == "alice";
TEND

ONLY_TEST("Basic declaration of war")
    ServerTestInterface s; s.run();
    ClientTestInterface alice("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);

    alice.sendMessage(CL_DECLARE_WAR, "bob");
    REPEAT_FOR_MS(500) {
        if (s.wars().isAtWar("alice", "bob"))
            return true;
    }
    return false;
TEND

ONLY_TEST("No erroneous wars")
    ServerTestInterface s; s.run();
    return ! s.wars().isAtWar("alice", "bob");
TEND

ONLY_TEST("Wars are persistent")
    {
        ServerTestInterface server1;
        server1.run();
        server1.wars().declare("alice", "bob");
    }
    ServerTestInterface *server2 = ServerTestInterface::KeepOldData();
    server2->run();
    bool result = server2->wars().isAtWar("alice", "bob");
    delete server2;
    return result;
TEND
