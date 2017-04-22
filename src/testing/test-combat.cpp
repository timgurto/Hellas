#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"
#include "../client/ClientNPC.h"

TEST("Players can attack immediately")
    TestServer s;
    s.loadData("testing/data/ant");
    s.run();

    TestClient c;
    c.loadData("testing/data/ant");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());

    // Add NPC
    s.addNPC("ant", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // Start attacking
    const auto objects = c.objects();
    const ClientObject *objP = objects.begin()->second;
    const ClientNPC &ant = dynamic_cast<const ClientNPC &>(*objP);
    size_t serial = ant.serial();
    c.sendMessage(CL_TARGET, makeArgs(serial));
    
    // NPC should be damaged very quickly
    REPEAT_FOR_MS(200)
        if (ant.health() < ant.npcType()->maxHealth())
            return true;
    return false;
TEND