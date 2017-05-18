#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
#include "../XmlReader.h"
#include "../client/ClientNPCType.h"

TEST("Read XML file with root only")
    XmlReader xr("testing/empty.xml");
    for (auto elem : xr.getChildren("nonexistent_tag"))
        ;
    return true;
TEND

TEST("Load object type naming an invalid item")
    TestServer s = TestServer::WithData("fake_item");
    auto it = s.items().find(ServerItem("fakeStone"));
    return it == s.items().end();
TEND

TEST("No crash on bad data")
    TestServer s = TestServer::WithData("this_doesnt_exist");
    return true;
TEND

TEST("Get spawn point from map file")
    TestServer s = TestServer::WithData("spawn_point_37");
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);
    const User &user = *s.users().begin();
    return user.location() == Point(37, 37);
TEND

TEST("Get spawn range from map file")
    TestServer s = TestServer::WithData("spawn_point_37ish");

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

TEST("Constructible NPC is loaded as NPC")
    // Load an item that refers to an object type, then an NPC type to define it
    TestClient c = TestClient::WithData("construct_an_npc");

    const ClientObjectType &objType = **c.objectTypes().begin();
    if (objType.classTag() != 'n')
        return false;

    // Check its health (to distinguish it from a plain ClientObject)
    const ClientNPCType &npcType = dynamic_cast<const ClientNPCType &>(objType);
    return npcType.maxHealth() == 5;
TEND

TEST("Object spawners work")
    // Given a spawner that maintains 3 rocks
    // When a server runs
    TestServer s = TestServer::WithData("spawned_rocks");

    // Then there are 3 rocks
    WAIT_UNTIL(s.entities().size() == 3);
    return true;
TEND

TEST("NPC spawners work")
    // Given a spawner that maintains 3 chickens
    // When a server runs
    TestServer s = TestServer::WithData("spawned_chickens");

    // Then there are 3 chickens
    WAIT_UNTIL(s.entities().size() == 3);
    return true;
TEND
