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

class ServerAndClientWithData {
 public:
  virtual ~ServerAndClientWithData() {
    delete client;
    delete server;
  }

  void useData(const char *dataString) {
    server = new TestServer(TestServer::WithDataString(dataString));
    client = new TestClient(TestClient::WithDataString(dataString));

    server->waitForUsers(1);
    user = &server->getFirstUser();
  }

  void useDataAndSpecificClass(const char *dataString, const char *userClass) {
    server = new TestServer(TestServer::WithDataString(dataString));
    client = new TestClient(
        TestClient::WithClassAndDataString(userClass, dataString));

    server->waitForUsers(1);
    user = &server->getFirstUser();
  }

  TestServer *server{nullptr};
  TestClient *client{nullptr};
  User *user{nullptr};
};

class ServerAndClientWithDataFiles {
 public:
  ~ServerAndClientWithDataFiles() {
    delete client;
    delete server;
  }

  void useData(const std::string &dataPath) {
    server = new TestServer(TestServer::WithData(dataPath));
    client = new TestClient(TestClient::WithData(dataPath));

    server->waitForUsers(1);
    user = &server->getFirstUser();
  }

  TestServer *server{nullptr};
  TestClient *client{nullptr};
  User *user{nullptr};
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

class TwoClientsWithData {
 public:
  ~TwoClientsWithData() {
    delete cAlice;
    delete cBob;
    delete server;
  }

  void useData(const char *dataString) {
    server = new TestServer(TestServer::WithDataString(dataString));
    cAlice = new TestClient(
        TestClient::WithUsernameAndDataString("Alice", dataString));
    cBob = new TestClient(
        TestClient::WithUsernameAndDataString("Bob", dataString));

    server->waitForUsers(2);
    uAlice = &server->findUser("Alice");
    uBob = &server->findUser("Bob");
  }

  TestServer *server{nullptr};
  TestClient *cAlice{nullptr}, *cBob{nullptr};
  User *uAlice{nullptr}, *uBob{nullptr};
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
