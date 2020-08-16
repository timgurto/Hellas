#pragma once

#include "TestClient.h"
#include "TestServer.h"

class TwoUsersNamedAliceAndBob {
 public:
  TwoUsersNamedAliceAndBob()
      : cAlice(TestClient::WithUsername("Alice")),
        cBob(TestClient::WithUsername("Bob")) {
    server.waitForUsers(2);
    alice = &server.findUser("Alice");
    bob = &server.findUser("Bob");
  }

  TestServer server;
  TestClient cAlice;
  TestClient cBob;

  User* alice;
  User* bob;
};
