#include "ClientTestInterface.h"
#include "ServerTestInterface.h"
#include "Test.h"

TEST("Recipes can be known by default")
    ServerTestInterface s;
    s.loadData("testing/data/box_from_nothing");
    s.run();
    ClientTestInterface c;
    c.loadData("testing/data/box_from_nothing");
    c.run();
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    c.sendMessage(CL_CRAFT, "box");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish
    return (user.inventory()[0].first->id() == "box");
TEND
