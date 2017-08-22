#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("City creation", "[city]"){
    TestServer s;
    s.cities().createCity("athens");
    CHECK(s.cities().doesCityExist("athens"));
}

TEST_CASE("No erroneous cities", "[city]"){
    TestServer s;
    CHECK_FALSE(s.cities().doesCityExist("Fakeland"));
}

TEST_CASE("Add a player to a city", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("athens");
    s.cities().addPlayerToCity(alice, "athens");

    CHECK(s.cities().isPlayerInCity("alice", "athens"));
}

TEST_CASE("No erroneous city membership", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    s.cities().createCity("athens");

    CHECK_FALSE(s.cities().isPlayerInCity("alice", "athens"));
}

TEST_CASE("Cities can't be overwritten", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("athens");
    s.cities().addPlayerToCity(alice, "athens");

    s.cities().createCity("athens");
    CHECK(s.cities().isPlayerInCity("alice", "athens"));
}

TEST_CASE("Default client knows no city membership", "[city]"){
    TestServer s;
    TestClient c;
    CHECK(c->character().cityName() == "");
}

TEST_CASE("Client is alerted to city membership", "[city][.flaky-vs2015]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("athens");
    s.cities().addPlayerToCity(alice, "athens");

    bool messageReceived = c.waitForMessage(SV_JOINED_CITY);
    REQUIRE(messageReceived);
    WAIT_UNTIL(c->character().cityName() == "athens");
}

TEST_CASE("Cities are persistent", "[city][persistence]"){
    {
        TestServer server1;
        TestClient client = TestClient::WithUsername("alice");
        WAIT_UNTIL(server1.users().size() == 1);
        User &alice = server1.getFirstUser();

        server1.cities().createCity("athens");
        server1.cities().addPlayerToCity(alice, "athens");
    }
    TestServer server2 = TestServer::KeepingOldData();

    CHECK(server2.cities().doesCityExist("athens"));
    CHECK(server2.cities().isPlayerInCity("alice", "athens"));
}

TEST_CASE("Clients are told if in a city on login", "[city]"){
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
}

TEST_CASE("Clients know nearby players' cities", "[.flaky][remote][city]"){
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
}

TEST_CASE("A player can cede an object to his city", "[.slow][city]"){
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
    CHECK_FALSE(rock.permissions().isOwnedByPlayer(user.name()));
}

TEST_CASE("A player must be in a city to cede", "[.slow][city]"){
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
    REQUIRE(c.waitForMessage(SV_NOT_IN_CITY));

    // And the object still belongs to the player
    CHECK(rock.permissions().isOwnedByPlayer(user.name()));
}

TEST_CASE("A player can only cede his own objects", "[.slow][city]") {
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
    CHECK(c.waitForMessage(SV_NO_PERMISSION, 10000));

    // And the object does not belong to Athens
    CHECK_FALSE(rock.permissions().isOwnedByCity("athens"));
}

TEST_CASE("A player can leave a city", "[city]") {
    // Given a user named Alice;
    auto c = TestClient::WithUsername("alice");
    auto s = TestServer{};
    WAIT_UNTIL(s.users().size() == 1);
    auto &user = s.getFirstUser();

    // Who is a member of Athens
    s.cities().createCity("athens");
    s.cities().addPlayerToCity(user, "athens");
    WAIT_UNTIL(s.cities().isPlayerInCity("alice", "athens"));

    // When the user sends a leave-city message
    c.sendMessage(CL_LEAVE_CITY);
}
