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

class TwoClients {
 public:
  TwoClients()
      : cAlice(TestClient::WithUsername("Alice")),
        cBob(TestClient::WithUsername("Bob")) {
    server.waitForUsers(2);
    alice = &server.findUser("Alice");
    bob = &server.findUser("Bob");
  }

  TestServer server;
  TestClient cAlice, cBob;
  User *alice, *bob;
};

class ThreeClients : public TwoClients {
 public:
  ThreeClients() : cCharlie(TestClient::WithUsername("Charlie")) {
    server.waitForUsers(3);
    charlie = &server.findUser("Charlie");
  }
  TestClient cCharlie;
  User *charlie;
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
