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

TEST("Server::loadData() replaces, not adds")
    TestServer s;
    s.loadData("testing/data/basic_rock");
    s.loadData("testing/data/basic_rock");
    return s.objectTypes().size() == 1;
TEND

TEST("Client::loadData() replaces, not adds")
    TestClient c;
    c.loadData("testing/data/basic_rock");
    c.loadData("testing/data/basic_rock");
    return c.objectTypes().size() == 1;
TEND

TEST("Load object type naming an invalid item")
    TestServer s;
    s.loadData("testing/data/fake_item");
    auto it = s.items().find(ServerItem("fakeStone"));
    return it == s.items().end();
TEND
