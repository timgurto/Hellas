#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("Gather an item from an object")
    TestServer s;
    s.loadData("testing/data/basic_rock");
    s.run();

    TestClient c = TestClient::Data("basic_rock");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add a single rock
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has item
    const Item &item = *s.items().begin();
    if (user.inventory()[0].first != &item)
        return false;

    //Make sure object no longer exists
    else if (!s.objects().empty())
        return false;

    return true;
TEND

/*
One gather worth of 1 million units of iron
1000 gathers worth of single rocks
This is to test the new gather algorithm, which would favor rocks rather than iron.
*/
TEST("Gather chance is by gathers, not quantity")
    TestServer s;
    s.loadData("testing/data/rare_iron");
    s.run();

    TestClient c = TestClient::Data("rare_iron");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add a single iron deposit
    s.addObject("ironDeposit", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has a rock, and not the iron
    const ServerItem &item = *s.items().find(ServerItem("rock"));
    return user.inventory()[0].first == &item;
TEND