#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Gather an item from an object"){
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
    user.updateLocation({ 10, 10 });

    // Add a single rock
    s.addObject("rock", { 10, 10 });
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has item
    const Item &item = *s.items().begin();
    CHECK(user.inventory()[0].first == &item);

    //Make sure object no longer exists
    CHECK(s.entities().empty());
}

/*
One gather worth of 1 million units of iron
1000 gathers worth of single rocks
This is to test the new gather algorithm, which would favor rocks rather than iron.
*/
TEST_CASE("Gather chance is by gathers, not quantity"){
    TestServer s = TestServer::WithData("rare_iron");
    TestClient c = TestClient::WithData("rare_iron");

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
    user.updateLocation({ 10, 10 });

    // Add a single iron deposit
    s.addObject("ironDeposit", { 10, 10 });
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has a rock, and not the iron
    const ServerItem &item = *s.items().find(ServerItem("rock"));
    CHECK(user.inventory()[0].first == &item);
}

TEST_CASE("Minimum yields"){
    TestServer s = TestServer::WithData("min_apples");
    for (auto entity : s.entities()){
        const Object *obj = dynamic_cast<const Object *>(entity);
        CHECK_FALSE(obj->contents().isEmpty());
    }
}
