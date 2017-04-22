#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
#include "../XmlReader.h"

TEST("Read XML file with root only")
    XmlReader xr("testing/empty.xml");
    for (auto elem : xr.getChildren("nonexistent_tag"))
        ;
    return true;
TEND

TEST("Load object type naming an invalid item")
    TestServer s = TestServer::Data("fake_item");
    auto it = s.items().find(ServerItem("fakeStone"));
    return it == s.items().end();
TEND

TEST("No crash on bad data")
    TestServer s = TestServer::Data("this_doesnt_exist");
    return true;
TEND

TEST("Get spawn point from map file")
    TestServer s = TestServer::Data("spawn_point_37");
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);
    const User &user = *s.users().begin();
    return user.location() == Point(37, 37);
TEND

TEST("Get spawn range from map file")
    TestServer s = TestServer::Data("spawn_point_37ish");

    RemoteClient
        c1("-username a"),
        c2("-username b"),
        c3("-username c");
    WAIT_UNTIL(s.users().size() == 3);

    for (const User &user : s.users())
        if (user.location().x < 17 ||
            user.location().y < 17 ||
            user.location().x > 57 ||
            user.location().y > 57)
                return false;
    return true;
TEND
