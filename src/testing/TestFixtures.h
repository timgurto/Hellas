#pragma once

#include "TestClient.h"
#include "TestServer.h"

class ServerAndClient {
 public:
  ServerAndClient() {
    server.waitForUsers(1);
    user = &server.getFirstUser();
  }

  TestServer server;
  TestClient client;
  User *user;
};

class ThreeClients {
 public:
  ThreeClients()
      : cAlice(TestClient::WithUsername("Alice")),
        cBob(TestClient::WithUsername("Bob")),
        cCharlie(TestClient::WithUsername("Charlie")) {
    server.waitForUsers(3);
    alice = &server.findUser("Alice");
    bob = &server.findUser("Bob");
    charlie = &server.findUser("Charlie");
  }

  TestServer server;
  TestClient cAlice, cBob, cCharlie;
  User *alice, *bob, *charlie;
};

class FourClients : public ThreeClients {
 public:
  FourClients() : cDan(TestClient::WithUsername("Dan")) {
    server.waitForUsers(4);
    dan = &server.findUser("Dan");
  }
  TestClient cDan;
  User *dan;
};
