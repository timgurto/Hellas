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

SLOW_TEST("A player can cede an object to his city")
    // Given a user in Athens;
    TestClient c = TestClient::WithData("basic_rock");
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.cities().addPlayerToCity(user, "athens");

    // And an object owned by him
    s.addObject("rock", Point(10, 10), user.name());

    // When he sends a CL_CEDE command
    WAIT_UNTIL(c.objects().size() == 1);
    Object &rock = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(rock.serial()));

    // Then the object belongs to Athens;
    WAIT_UNTIL(rock.permissions().isOwnedByCity("athens"));

    // And the object doesn't belong to him.
    return ! rock.permissions().isOwnedByPlayer(user.name());
TEND

SLOW_TEST("A player must be in a city to cede")
    // Given a user who owns a rock
    TestClient c = TestClient::WithData("basic_rock");
    TestServer s = TestServer::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.addObject("rock", Point(10, 10), user.name());

    // When he sends a CL_CEDE command
    WAIT_UNTIL(c.objects().size() == 1);
    Object &rock = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(rock.serial()));

    // Then the player receives an error message;
    if (!c.waitForMessage(SV_NOT_IN_CITY))
        return false;

    // And the object still belongs to the player
    return rock.permissions().isOwnedByPlayer(user.name());
TEND

TEST("A player can only cede his own objects")
    // Given a user in Athens;
    TestClient c = TestClient::WithData("basic_rock");
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("athens");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.cities().addPlayerToCity(user, "athens");

    // And a rock owned by nobody
    s.addObject("rock", Point(10, 10));

    // When he sends a CL_CEDE command
    WAIT_UNTIL(c.objects().size() == 1);
    Object &rock = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(rock.serial()));
    
    // Then the player receives an error message;
    if (!c.waitForMessage(SV_NO_PERMISSION, 10000))
        return false;

    // And the object does not belong to Athens
    return ! rock.permissions().isOwnedByCity("athens");
TEND
