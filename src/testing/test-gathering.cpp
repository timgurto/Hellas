#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Gather an item from an object") {
  auto s = TestServer::WithData("basic_rock");
  auto c = TestClient::WithData("basic_rock");

  // Add a single rock
  s.addObject("rock", {10, 10});
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // Gather
  auto serial = c.objects().begin()->first;
  c.sendMessage(CL_GATHER, makeArgs(serial));
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.action() ==
             User::Action::GATHER);  // Wait for gathering to start
  WAIT_UNTIL(user.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  // Make sure user has item
  const auto &item = *s.items().begin();
  CHECK(user.inventory()[0].first.type() == &item);

  // Make sure object no longer exists
  CHECK(s.entities().empty());
}

/*
One gather worth of 1 million units of iron
1000 gathers worth of single rocks
This is to test the new gather algorithm, which would favor rocks rather than
iron. It will fail randomly about 1 every 1000 times
*/
TEST_CASE("Gather chance is by gathers, not quantity") {
  auto s = TestServer::WithData("rare_iron");
  auto c = TestClient::WithData("rare_iron");

  // Add a single iron deposit
  s.addObject("ironDeposit", {10, 10});
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // Gather
  auto serial = c.objects().begin()->first;
  c.sendMessage(CL_GATHER, makeArgs(serial));
  auto &user = s.getFirstUser();
  WAIT_UNTIL(user.action() ==
             User::Action::GATHER);  // Wait for gathering to start
  WAIT_UNTIL(user.action() ==
             User::Action::NO_ACTION);  // Wait for gathering to finish

  // Make sure user has a rock, and not the iron
  const auto &item = *s.items().find(ServerItem("rock"));
  CHECK(user.inventory()[0].first.type() == &item);
}

TEST_CASE("Minimum yields") {
  auto s = TestServer::WithData("min_apples");
  for (auto entity : s.entities()) {
    const Object *obj = dynamic_cast<const Object *>(entity);
    CHECK_FALSE(obj->gatherContents().isEmpty());
  }
}
