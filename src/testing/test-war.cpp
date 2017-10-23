#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Basic declaration of war", "[war]"){
    // Given Alice is logged in
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice sends a CL_DECLARE_WAR_ON_PLAYER message
    alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "bob");

    // Then Alice and Bob go to war
    WAIT_UNTIL(s.wars().isAtWar("alice", "bob"));
}

TEST_CASE("No erroneous wars", "[war]"){
    // When a clean server is started
    TestServer s;
    
    // Then Alice and Bob are not at war
    CHECK_FALSE(s.wars().isAtWar("alice", "bob"));
}

TEST_CASE("Wars are persistent", "[war][persistence]"){
    // Given Alice and Bob are at war, and there is no server running
    {
        TestServer server1;
        server1.wars().declare("alice", "bob");
    }

    // When a server begins that keeps persistent data
    TestServer server2 = TestServer::KeepingOldData();

    // Then Alice and Bob are still at war
    CHECK(server2.wars().isAtWar("alice", "bob"));
}

TEST_CASE("Clients are alerted of new wars", "[war][remote]"){
    // Given Alice is logged in
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // And Bob is logged in
    RemoteClient rcBob("-username bob");
    WAIT_UNTIL(s.users().size() == 2);

    // When Alice declares war on Bob
    alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "bob");

    // Then Alice is alerted to the new war
    WAIT_UNTIL(alice.otherUsers().size() == 1);
    auto bob = alice.getFirstOtherUser();
    WAIT_UNTIL(alice->isAtWarWith(bob));
}

TEST_CASE("Clients are told of existing wars on login", "[war][remote]"){
    // Given Alice and Bob are at war
    TestServer s;
    s.wars().declare("alice", "bob");

    // When Alice and Bob log in
    TestClient alice = TestClient::WithUsername("alice");
    RemoteClient rcBob("-username bob");
    WAIT_UNTIL(s.users().size() == 1);

    // Then she is told about the war
    WAIT_UNTIL(alice.otherUsers().size() == 1);
    auto bob = alice.getFirstOtherUser();
    WAIT_UNTIL(alice->isAtWarWith(bob));
}

TEST_CASE("Wars cannot be redeclared", "[war]"){
    // Given Alice and Bob are at war, and Alice is logged in
    TestServer s;
    TestClient alice = TestClient::WithUsername("alice");
    s.wars().declare("alice", "bob");
    WAIT_UNTIL(s.users().size() == 1);

    // When Alice declares war on Bob
    alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "bob");

    // Then she receives an SV_ALREADY_AT_WAR error message
    CHECK(alice.waitForMessage(SV_ALREADY_AT_WAR));
}

TEST_CASE("A player can be at war with a city", "[war][city]"){
    // Given a running server;
    TestServer s;

    // And a city named Athens;
    s.cities().createCity("athens");

    // And a user named Alice
    TestClient c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When a war is declared between Alice and Athens
    s.wars().declare("alice", "athens");

    // Then they are considered to be at war.
    CHECK(s.wars().isAtWar("alice", "athens"));
}

TEST_CASE("A player at war with a city is at war with its members", "[war][city][remote]"){
    // Given a running server;
    TestServer s;

    // And a city named Athens;
    s.cities().createCity("athens");

    // And a user, Alice, who is a member of Athens;
    RemoteClient alice("-username alice");
    WAIT_UNTIL(s.users().size() == 1);
    s.cities().addPlayerToCity(s.getFirstUser(), "athens");

    // When new user Bob and Athens go to war
    Belligerent
        b1("bob", Belligerent::PLAYER),
        b2("athens", Belligerent::CITY);

    SECTION("Bob logs in, then war is declared"){
        TestClient bob = TestClient::WithUsername("bob");
        WAIT_UNTIL(s.users().size() == 2);
        s.wars().declare(b1, b2);
        
        // Then Bob is at war with Alice;
        CHECK(s.wars().isAtWar("alice", "bob"));

        // And Bob knows that he's at war with Alice
        WAIT_UNTIL(bob.otherUsers().size() == 1);
        const auto &cAlice = bob.getFirstOtherUser();
        WAIT_UNTIL(bob.isAtWarWith(cAlice));
    }

    SECTION("War is declared, then Bob logs in"){
        s.wars().declare(b1, b2);
        WAIT_UNTIL(s.wars().isAtWar(b1, b2));
        TestClient bob = TestClient::WithUsername("bob");
        
        // Then Bob is at war with Alice;
        CHECK(s.wars().isAtWar("alice", "bob"));

        // And Bob knows that he's at war with Alice
        WAIT_UNTIL(bob.otherUsers().size() == 1);
        const auto &cAlice = bob.getFirstOtherUser();
        WAIT_UNTIL(bob.isAtWarWith(cAlice));
    }
}

TEST_CASE("Players can declare war on cities", "[war][city]"){
    // Given a running server;
    TestServer s;

    // And a city named Athens;
    s.cities().createCity("athens");

    // And a user, Alice;
    TestClient alice = TestClient::WithUsername("alice");

    // When Alice declares war on Athens
    alice.sendMessage(CL_DECLARE_WAR_ON_CITY, "athens");

    // Then they are at war
    Belligerent
        b1("alice", Belligerent::PLAYER),
        b2("athens", Belligerent::CITY);
    WAIT_UNTIL(s.wars().isAtWar(b1, b2));
}

TEST_CASE("Wars involving cities are persistent", "[persistence][city][war]"){
    Belligerent
        b1("alice", Belligerent::PLAYER),
        b2("athens", Belligerent::CITY);

    {
        // Given a city named Athens;
        TestServer server1;
        server1.cities().createCity("athens");

        // And Alice and Athens are at war;
        server1.wars().declare(b1, b2);

        // And there is no server running
    }

    // When a server begins that keeps persistent data
    TestServer server2 = TestServer::KeepingOldData();

    // Then Alice and Athens are still at war
    CHECK(server2.wars().isAtWar(b1, b2));
}

TEST_CASE("The objects of an offline enemy in an enemy city can be attacked", "[.slow][city][war][remote]"){
    // Given a server with rock objects;
    TestServer s = TestServer::WithData("chair");

    // And a city named Athens;
    s.cities().createCity("athens");

    // And Bob is a member of Athens;
    {
        RemoteClient bob("-username bob -data testing/data/chair");
        WAIT_UNTIL(s.users().size() == 1);
        s.cities().addPlayerToCity(s.getFirstUser(), "athens");

    // And Bob is offline
    }
    WAIT_UNTIL(s.users().size() == 0);

    // And a rock owned by Bob;
    s.addObject("chair", Point(15,15), "bob");

    // And a player, Alice;
    TestClient alice = TestClient::WithUsernameAndData("alice", "chair");

    // And Alice is at war with Athens
    s.wars().declare("alice", Belligerent("athens", Belligerent::CITY));

    // When Alice becomes aware of the rock
    WAIT_UNTIL(alice.objects().size() == 1);

    // Then Alice can attack the rock
    const auto &rock = alice.getFirstObject();
    WAIT_UNTIL(rock.canBeAttackedByPlayer());
}

TEST_CASE("A player is alerted when he sues for peace", "[war][peace]") {
    // Given Alice and Bob are at war
    auto s = TestServer{};
    s.wars().declare({ "alice", Belligerent::PLAYER }, { "bob", Belligerent::PLAYER });

    // When Alice sues for peace
    auto c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);
    c.sendMessage(CL_SUE_FOR_PEACE_WITH_PLAYER, "bob");

    // Then Alice is alerted
    CHECK(c.waitForMessage(SV_YOU_PROPOSED_PEACE));
}

TEST_CASE("The enemy is alerted when peace is proposed", "[war][peace][remote]") {
    // Given Alice and Bob are at war
    auto s = TestServer{};
    s.wars().declare({ "alice", Belligerent::PLAYER }, { "bob", Belligerent::PLAYER });

    // And Alice is logged in
    auto c = TestClient::WithUsername("alice");
    WAIT_UNTIL(s.users().size() == 1);

    // When Bob logs in and sues for peace
    auto rc = RemoteClient{ "-username bob" };
    WAIT_UNTIL(s.users().size() == 2);
    auto &bob = s.findUser("bob");
    s->handleMessage(bob.socket(), s->compileMessage(CL_SUE_FOR_PEACE_WITH_PLAYER, "alice"));

    // Then Alice is alerted
    CHECK(c.waitForMessage(SV_PEACE_WAS_PROPOSED_TO_YOU));
}

TEST_CASE("Users are alerted to peace proposals on login", "[war][peace]") {
    // Given Alice and Bob are at war
    auto s = TestServer{};
    s.wars().declare({ "alice", Belligerent::PLAYER }, { "bob", Belligerent::PLAYER });

    {
        // When Alice sues for peace
        auto c = TestClient::WithUsername("alice");
        WAIT_UNTIL(s.users().size() == 1);
        c.sendMessage(CL_SUE_FOR_PEACE_WITH_PLAYER, "bob");

        // And disconnects
    }

    SECTION("Alice logs in") {
        auto c = TestClient::WithUsername("alice");
        WAIT_UNTIL(s.users().size() == 1);

        // Then Alice is alerted
        CHECK(c.waitForMessage(SV_YOU_PROPOSED_PEACE));
    }

    SECTION("Bob logs in") {
        auto c = TestClient::WithUsername("bob");
        WAIT_UNTIL(s.users().size() == 1);

        // Then Bob is alerted
        CHECK(c.waitForMessage(SV_PEACE_WAS_PROPOSED_TO_YOU));
    }
}
