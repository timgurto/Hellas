#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Recipes can be known by default")
    TestServer s;
    s.loadData("testing/data/box_from_nothing");
    s.run();
    TestClient c;
    c.loadData("testing/data/box_from_nothing");
    c.run();
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    c.sendMessage(CL_CRAFT, "box");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish
    return (user.inventory()[0].first->id() == "box");
TEND

TEST("Terrain as tool")
    TestServer s;
    s.loadData("testing/data/daisy_chain");
    s.run();
    TestClient c;
    c.loadData("testing/data/daisy_chain");
    c.run();
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    c.sendMessage(CL_CRAFT, "daisyChain");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish
    return (user.inventory()[0].first->id() == "daisyChain");
TEND

TEST("Client sees default recipes")
    TestServer s;
    s.loadData("testing/data/box_from_nothing");
    s.run();
    TestClient c;
    c.loadData("testing/data/box_from_nothing");
    c.run();
    WAIT_UNTIL (s.users().size() == 1);
    c.showCraftingWindow();
    return c.recipeList().size() == 1;
TEND
