#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Recipes can be known by default"){
    TestServer s = TestServer::WithData("box_from_nothing");
    TestClient c = TestClient::WithData("box_from_nothing");
    WAIT_UNTIL (s.users().size() == 1);

    User &user = s.getFirstUser();
    c.sendMessage(CL_CRAFT, "box");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    const ServerItem *itemInFirstSlot = user.inventory()[0].first;
    REQUIRE(itemInFirstSlot != nullptr);
    CHECK(itemInFirstSlot->id() == "box");
}

TEST_CASE("Terrain as tool"){
    TestServer s = TestServer::WithData("daisy_chain");
    TestClient c = TestClient::WithData("daisy_chain");
    WAIT_UNTIL (s.users().size() == 1);

    User &user = s.getFirstUser();
    c.sendMessage(CL_CRAFT, "daisyChain");
    WAIT_UNTIL (user.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    const ServerItem *itemInFirstSlot = user.inventory()[0].first;
    REQUIRE(itemInFirstSlot != nullptr);
    CHECK(itemInFirstSlot->id() == "daisyChain");
}

TEST_CASE("Client sees default recipes"){
    TestServer s = TestServer::WithData("box_from_nothing");
    TestClient c = TestClient::WithData("box_from_nothing");
    WAIT_UNTIL (s.users().size() == 1);

    c.showCraftingWindow();

    CHECK(c.recipeList().size() == 1);
}

TEST_CASE("Crafting is allowed if materials will vacate a slot"){
    // Given a server and client;
    // And items/recipe for meat -> cooked meat;
    TestServer s = TestServer::WithData("cooking_meat");
    TestClient c = TestClient::WithData("cooking_meat");

    // And the user has an inventory full of meat
    WAIT_UNTIL(s.users().size() == 1);
    User &u = s.getFirstUser();
    const ServerItem &meat = *s.items().find(ServerItem("meat"));
    u.giveItem(&meat, User::INVENTORY_SIZE);
    
    // When he tries to craft cooked meat
    c.sendMessage(CL_CRAFT, "cookedMeat");
    WAIT_UNTIL (u.action() == User::Action::CRAFT) ; // Wait for gathering to start
    WAIT_UNTIL (u.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    // Then his inventory contains cooked meat
    const ServerItem *itemInFirstSlot = u.inventory()[0].first;
    REQUIRE(itemInFirstSlot != nullptr);
    CHECK(itemInFirstSlot->id() == "cookedMeat");
}
