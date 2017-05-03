#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"

TEST("City creation")
    TestServer s;
    s.cities().createCity("Athens");
    return s.cities().doesCityExist("Athens");
TEND

TEST("No erroneous cities")
    TestServer s;
    return ! s.cities().doesCityExist("Fakeland");
TEND

TEST("Add a player to a city")
    TestServer s;
    TestClient c = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(alice, "Athens");

    return s.cities().isPlayerInCity("alice", "Athens");
TEND

TEST("No erroneous city membership")
    TestServer s;
    TestClient c = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);

    s.cities().createCity("Athens");

    return ! s.cities().isPlayerInCity("alice", "Athens");
TEND

TEST("Cities can't be overwritten")
    TestServer s;
    TestClient c = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(alice, "Athens");

    s.cities().createCity("Athens");
    return s.cities().isPlayerInCity("alice", "Athens");
TEND

TEST("Default client knows no city membership")
    TestServer s;
    TestClient c;
    return c->cityName() == "";
TEND

TEST("Client is alerted to city membership")
    TestServer s;
    TestClient c = TestClient::Username("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(alice, "Athens");

    bool messageReceived = c.waitForMessage(SV_JOINED_CITY);
    if (! messageReceived)
        return false;
    WAIT_UNTIL(c->cityName() == "Athens");

    return true;
TEND

TEST("Cities are persistent")
    {
        TestServer server1;
        TestClient client = TestClient::Username("alice");
        WAIT_UNTIL(server1.users().size() == 1);
        User &alice = server1.getFirstUser();

        server1.cities().createCity("Athens");
        server1.cities().addPlayerToCity(alice, "Athens");
    }
    TestServer server2 = TestServer::KeepOldData();

    if (! server2.cities().doesCityExist("Athens"))
        return false;
    if (! server2.cities().isPlayerInCity("alice", "Athens"))
        return false;

    return true;
TEND

// Previous users persisting as objects causes this test to crash sometimes.
QUARANTINED_TEST("Clients are told if in a city on login")
    TestServer server;
    server.cities().createCity("Athens");
    {
        TestClient client1 = TestClient::Username("alice");
        WAIT_UNTIL(server.users().size() == 1);
        User &alice = server.getFirstUser();
        server.cities().addPlayerToCity(alice, "Athens");
    }

    TestClient client2 = TestClient::Username("alice");
    WAIT_UNTIL(client2->cityName() == "Athens");

    return true;
TEND
