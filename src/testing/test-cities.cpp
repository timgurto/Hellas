#include "RemoteClient.h"
#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("City creation")
    TestServer s;
    s.cities().createCity("athens");
    return s.cities().doesCityExist("athens");
TEND

TEST("No erroneous cities")
    TestServer s;
    return ! s.cities().doesCityExist("Fakeland");
TEND

TEST("Add a player to a city")
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("athens");
    s.cities().addPlayerToCity(alice, "athens");

    return s.cities().isPlayerInCity("alice", "athens");
TEND

TEST("No erroneous city membership")
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    s.cities().createCity("athens");

    return ! s.cities().isPlayerInCity("alice", "athens");
TEND

TEST("Cities can't be overwritten")
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("athens");
    s.cities().addPlayerToCity(alice, "athens");

    s.cities().createCity("athens");
    return s.cities().isPlayerInCity("alice", "athens");
TEND

TEST("Default client knows no city membership")
    TestServer s;
    TestClient c;
    return c->character().cityName() == "";
TEND

TEST("Client is alerted to city membership")
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("athens");
    s.cities().addPlayerToCity(alice, "athens");

    bool messageReceived = c.waitForMessage(SV_JOINED_CITY);
    if (! messageReceived)
        return false;
    WAIT_UNTIL(c->character().cityName() == "athens");

    return true;
TEND

TEST("Cities are persistent")
    {
        TestServer server1;
        TestClient client = TestClient::WithUsername("alice");
        WAIT_UNTIL(server1.users().size() == 1);
        User &alice = server1.getFirstUser();

        server1.cities().createCity("athens");
        server1.cities().addPlayerToCity(alice, "athens");
    }
    TestServer server2 = TestServer::KeepingOldData();

    if (! server2.cities().doesCityExist("athens"))
        return false;
    if (! server2.cities().isPlayerInCity("alice", "athens"))
        return false;

    return true;
TEND

TEST("Clients are told if in a city on login")
    TestServer server;
    server.cities().createCity("athens");
    {
        TestClient client1 = TestClient::WithUsername("alice");
        WAIT_UNTIL(server.users().size() == 1);
        User &alice = server.getFirstUser();
        server.cities().addPlayerToCity(alice, "athens");
    }

    TestClient client2 = TestClient::WithUsername("alice");
    WAIT_UNTIL(client2->character().cityName() == "athens");

    return true;
TEND

QUARANTINED_TEST("Clients know nearby players' cities")
    // Given Alice is a member of Athens, and connected to the server
    TestServer s;
    s.cities().createCity("athens");
    RemoteClient rc("-username alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &serverAlice = s.getFirstUser();
    s.cities().addPlayerToCity(serverAlice, "athens");

    // When another client connects
    TestClient c;
    WAIT_UNTIL(c.otherUsers().size() == 1);

    // Then that client can see that Alice is in Athens
    const Avatar &clientAlice = c.getFirstOtherUser();
    WAIT_UNTIL(clientAlice.cityName() == "athens");
    return true;
TEND
