#include "ClientTestInterface.h"
#include "RemoteClient.h"
#include "Test.h"
#include "ServerTestInterface.h"

ONLY_TEST("Run a client in a separate process")
    ServerTestInterface s;
    s.run();
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 1);
    return true;
TEND
ONLY_TEST("Run CTI with custom username")
    ServerTestInterface s; s.run();
    ClientTestInterface alice("alice"); alice.run();
    WAIT_UNTIL(s.users().size() == 1);
    return alice->username() == "alice";
TEND

