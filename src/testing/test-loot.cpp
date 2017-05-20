#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

ONLY_TEST("Client gets loot info and can loot")
    // Given an NPC that always drops 1 gold
    TestServer s = TestServer::WithData("goldbug");
    s.addNPC("goldbug", Point(10, 15));

    // When a user kills it
    TestClient c = TestClient::WithData("goldbug");
    WAIT_UNTIL(c.objects().size() == 1);
    NPC &goldbug = s.getFirstNPC();
    c.sendMessage(CL_TARGET_NPC, makeArgs(goldbug.serial()));
    WAIT_UNTIL(goldbug.health() == 0);

    // Then the user can see one item in its the loot window;
    ClientNPC &clientGoldbug = c.getFirstNPC();
    c.watchObject(clientGoldbug);
    WAIT_UNTIL(clientGoldbug.container().size() == 1);

    // And the server survives a loot request
    c.sendMessage(CL_TAKE_ITEM, makeArgs(goldbug.serial(), 0));
    s.entities();

    return true;
TEND
