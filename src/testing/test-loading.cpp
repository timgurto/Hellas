#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"
#include "../XmlReader.h"
#include "../client/ClientNPCType.h"

TEST_CASE("Read XML file with root only"){
    XmlReader xr("testing/empty.xml");
    for (auto elem : xr.getChildren("nonexistent_tag"))
        ;
}

TEST_CASE("Load object type naming an invalid item"){
    TestServer s = TestServer::WithData("fake_item");
    auto it = s.items().find(ServerItem("fakeStone"));
    CHECK(it == s.items().end());
}

TEST_CASE("No crash on bad data"){
    TestServer s = TestServer::WithData("this_doesnt_exist");
}

TEST_CASE("Get spawn point from map file"){
    TestServer s = TestServer::WithData("spawn_point_37");
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);
    const User &user = *s.users().begin();
    CHECK(user.location() == MapPoint(37, 37));
}

TEST_CASE("Get spawn range from map file", "[remote]"){
    TestServer s = TestServer::WithData("spawn_point_37ish");

    RemoteClient
        c1("-username Aaa"),
        c2("-username Bbb"),
        c3("-username Ccc");
    WAIT_UNTIL(s.users().size() == 3);

    for (const User &user : s.users()){
        CHECK(user.location().x > 17);
        CHECK(user.location().y > 17);
        CHECK(user.location().x < 57);
        CHECK(user.location().y < 57);
    }
}

TEST_CASE("Constructible NPC is loaded as NPC"){
    // Load an item that refers to an object type, then an NPC type to define it
    TestClient c = TestClient::WithData("construct_an_npc");

    const ClientObjectType &objType = **c.objectTypes().begin();
    REQUIRE(objType.classTag() == 'n');

    // Check its health (to distinguish it from a plain ClientObject)
    const ClientNPCType &npcType = dynamic_cast<const ClientNPCType &>(objType);
    CHECK(npcType.maxHealth() == 5);
}

TEST_CASE("Object spawners work"){
    // Given a spawner that maintains 3 rocks
    // When a server runs
    TestServer s = TestServer::WithData("spawned_rocks");

    // Then there are 3 rocks
    WAIT_UNTIL(s.entities().size() == 3);
}

TEST_CASE("NPC spawners work"){
    // Given a spawner that maintains 3 chickens
    // When a server runs
    TestServer s = TestServer::WithData("spawned_chickens");

    // Then there are 3 chickens
    WAIT_UNTIL(s.entities().size() == 3);
}

TEST_CASE("Clients load map properly") {
    // Given a server and client, with a 101x101 map on which users spawn at the midpoint
    TestServer s = TestServer::WithData("big_map");
    TestClient c = TestClient::WithData("big_map");

    // Then the client's map has 101 rows;
    WAIT_UNTIL(c.map().size() == 101);

    // And the user spawned in the correct place
    WAIT_UNTIL(c->character().location() == MapPoint(1616, 1616));
}

TEST_CASE("Help text is valid XML") {
    auto c = TestClient{};
    const auto &helpTextEntries = c->helpEntries();
    CHECK(helpTextEntries.begin() != helpTextEntries.end());
}
