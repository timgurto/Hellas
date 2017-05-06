#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Recipes can be known by default")
    TestServer s = TestServer::WithData("box_from_nothing");
    TestClient c = TestClient::WithData("box_from_nothing");
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
    c.sendMessage(CL_CRAFT, "box");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish
    return (user.inventory()[0].first->id() == "box");
TEND

TEST("Terrain as tool")
    TestServer s = TestServer::WithData("daisy_chain");
    TestClient c = TestClient::WithData("daisy_chain");
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
    c.sendMessage(CL_CRAFT, "daisyChain");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish
    return (user.inventory()[0].first->id() == "daisyChain");
TEND

TEST("Client sees default recipes")
    TestServer s = TestServer::WithData("box_from_nothing");
    TestClient c = TestClient::WithData("box_from_nothing");
    WAIT_UNTIL (s.users().size() == 1);
    c.showCraftingWindow();
    return c.recipeList().size() == 1;
TEND
