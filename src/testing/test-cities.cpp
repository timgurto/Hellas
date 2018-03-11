#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("City creation", "[city]"){
    TestServer s;
    s.cities().createCity("Athens");
    CHECK(s.cities().doesCityExist("Athens"));
}

TEST_CASE("No erroneous cities", "[city]"){
    TestServer s;
    CHECK_FALSE(s.cities().doesCityExist("Fakeland"));
}

TEST_CASE("Add a player to a city", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("Alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(alice, "Athens");

    CHECK(s.cities().isPlayerInCity("Alice", "Athens"));
}

TEST_CASE("No erroneous city membership", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("Alice");
    WAIT_UNTIL(s.users().size() == 1);

    s.cities().createCity("Athens");

    CHECK_FALSE(s.cities().isPlayerInCity("Alice", "Athens"));
}

TEST_CASE("Cities can't be overwritten", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("Alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(alice, "Athens");

    s.cities().createCity("Athens");
    CHECK(s.cities().isPlayerInCity("Alice", "Athens"));
}

TEST_CASE("Default client knows no city membership", "[city]"){
    TestServer s;
    TestClient c;
    CHECK(c->character().cityName() == "");
}

TEST_CASE("Client is alerted to city membership", "[city]"){
    TestServer s;
    TestClient c = TestClient::WithUsername("Alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &alice = s.getFirstUser();

    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(alice, "Athens");

    bool messageReceived = c.waitForMessage(SV_JOINED_CITY);
    REQUIRE(messageReceived);
    WAIT_UNTIL(c->character().cityName() == "Athens");
}

TEST_CASE("Cities are persistent", "[city][persistence]"){
    {
        TestServer server1;
        TestClient client = TestClient::WithUsername("Alice");
        WAIT_UNTIL(server1.users().size() == 1);
        User &alice = server1.getFirstUser();

        server1.cities().createCity("Athens");
        server1.cities().addPlayerToCity(alice, "Athens");
    }
    TestServer server2 = TestServer::KeepingOldData();

    CHECK(server2.cities().doesCityExist("Athens"));
    CHECK(server2.cities().isPlayerInCity("Alice", "Athens"));
}

TEST_CASE("Clients are told if in a city on login", "[city]"){
    TestServer server;
    server.cities().createCity("Athens");
    {
        TestClient client1 = TestClient::WithUsername("Alice");
        WAIT_UNTIL(server.users().size() == 1);
        User &alice = server.getFirstUser();
        server.cities().addPlayerToCity(alice, "Athens");
    }

    TestClient client2 = TestClient::WithUsername("Alice");
    WAIT_UNTIL(client2->character().cityName() == "Athens");
}

TEST_CASE("Clients know nearby players' cities", "[.flaky][remote][city]"){
    // Given Alice is a member of Athens, and connected to the server
    TestServer s;
    s.cities().createCity("Athens");
    RemoteClient rc("-username Alice");
    WAIT_UNTIL(s.users().size() == 1);
    User &serverAlice = s.getFirstUser();
    s.cities().addPlayerToCity(serverAlice, "Athens");

    // When another client connects
    TestClient c;
    WAIT_UNTIL(c.otherUsers().size() == 1);

    // Then that client can see that Alice is in Athens
    const Avatar &clientAlice = c.getFirstOtherUser();
    WAIT_UNTIL(clientAlice.cityName() == "Athens");
}

TEST_CASE("A player can cede an object to his city", "[.slow][city]"){
    // Given a user in Athens;
    TestClient c = TestClient::WithData("basic_rock");
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("Athens");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.cities().addPlayerToCity(user, "Athens");

    // And an object owned by him
    s.addObject("rock", { 10, 10 }, user.name());

    // When he sends a CL_CEDE command
    WAIT_UNTIL(c.objects().size() == 1);
    Object &rock = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(rock.serial()));

    // Then the object belongs to Athens;
    WAIT_UNTIL(rock.permissions().isOwnedByCity("Athens"));

    // And the object doesn't belong to him.
    CHECK_FALSE(rock.permissions().isOwnedByPlayer(user.name()));
}

TEST_CASE("A player must be in a city to cede", "[.slow][city]"){
    // Given a user who owns a rock
    TestClient c = TestClient::WithData("basic_rock");
    TestServer s = TestServer::WithData("basic_rock");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.addObject("rock", { 10, 10 }, user.name());

    // When he sends a CL_CEDE command
    WAIT_UNTIL(c.objects().size() == 1);
    Object &rock = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(rock.serial()));

    // Then the player receives an error message;
    REQUIRE(c.waitForMessage(ERROR_NOT_IN_CITY));

    // And the object still belongs to the player
    CHECK(rock.permissions().isOwnedByPlayer(user.name()));
}

TEST_CASE("A player can only cede his own objects", "[.slow][city]") {
    // Given a user in Athens;
    TestClient c = TestClient::WithData("basic_rock");
    TestServer s = TestServer::WithData("basic_rock");
    s.cities().createCity("Athens");
    WAIT_UNTIL(s.users().size() == 1);
    User &user = s.getFirstUser();
    s.cities().addPlayerToCity(user, "Athens");

    // And a rock owned by nobody
    s.addObject("rock", { 10, 10 });

    // When he sends a CL_CEDE command
    WAIT_UNTIL(c.objects().size() == 1);
    Object &rock = s.getFirstObject();
    c.sendMessage(CL_CEDE, makeArgs(rock.serial()));

    // Then the player receives an error message;
    CHECK(c.waitForMessage(WARNING_NO_PERMISSION, 10000));

    // And the object does not belong to Athens
    CHECK_FALSE(rock.permissions().isOwnedByCity("Athens"));
}

TEST_CASE("A player can leave a city", "[city]") {
    // Given a user named Alice;
    auto c = TestClient::WithUsername("Alice");
    auto s = TestServer{};
    WAIT_UNTIL(s.users().size() == 1);
    auto &user = s.getFirstUser();

    // Who is a member of Athens;
    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(user, "Athens");
    WAIT_UNTIL(s.cities().isPlayerInCity("Alice", "Athens"));

    // And who knows it
    WAIT_UNTIL(c.cityName() == "Athens");

    SECTION("When Alice sends a leave-city message") {
        c.sendMessage(CL_LEAVE_CITY);
    }
    SECTION("When Alice enters \"/cquit\" into the chat") {
        c.performCommand("/cquit");
    }

    // Then Alice is not in a city;
    WAIT_UNTIL(!s.cities().isPlayerInCity("Alice", "Athens"));
    CHECK(s.cities().getPlayerCity("Alice").empty());

    // And the user knows it
    WAIT_UNTIL(c.cityName().empty());
}

TEST_CASE("A player can't leave a city if not in one", "[city]") {
    // Given a user;
    auto c = TestClient{};
    auto s = TestServer{};
    WAIT_UNTIL(s.users().size() == 1);
    auto &user = s.getFirstUser();

    // When the user sends a leave-city message
    c.sendMessage(CL_LEAVE_CITY);

    // Then the player receives an error message;
    CHECK(c.waitForMessage(ERROR_NOT_IN_CITY));
}

TEST_CASE("A king can't leave his city", "[city][king]") {
    // Given a user named Alice;
    auto c = TestClient::WithUsername("Alice");
    auto s = TestServer{};
    WAIT_UNTIL(s.users().size() == 1);
    auto &user = s.getFirstUser();

    // Who is a member of Athens;
    s.cities().createCity("Athens");
    s.cities().addPlayerToCity(user, "Athens");
    WAIT_UNTIL(s.cities().isPlayerInCity("Alice", "Athens"));

    // And is a king
    s->makePlayerAKing(user);

    // When Alice sends a leave-city message
    c.sendMessage(CL_LEAVE_CITY);

    // Then Alice receives an error message;
    CHECK(c.waitForMessage(ERROR_KING_CANNOT_LEAVE_CITY));

    // And Alice is still in Athens
    REPEAT_FOR_MS(100);
    CHECK(s.cities().isPlayerInCity("Alice", "Athens"));
}

TEST_CASE("Kingship is persistent", "[king]") {
    // Given a user named Alice;
    {
        auto c = TestClient::WithUsername("Alice");
        auto s = TestServer{};
        WAIT_UNTIL(s.users().size() == 1);
        auto &user = s.getFirstUser();

        // Who is a king
        s->makePlayerAKing(user);

        // When the server restarts
    }
    {
        auto c = TestClient::WithUsername("Alice");
        auto s = TestServer::KeepingOldData();
        WAIT_UNTIL(s.users().size() == 1);

        // Then Alice is still a king
        WAIT_UNTIL(s.kings().isPlayerAKing("Alice"));
    }
}
