#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Class can be specified in TestClients") {
  GIVEN("Two classes, Class1 and Class2") {
    auto data = R"(
      <class name="Class1" />
      <class name="Class2" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a client is created as Class2") {
      auto c = TestClient::WithClassAndDataString("Class2", data);
      s.waitForUsers(1);

      THEN("his class is 'Class2' according to the server") {
        const auto &user = s.getFirstUser();
        CHECK(user.getClass().type().id() == "Class2");
      }
    }
  }
}

TEST_CASE("Talents can be taken") {
  GIVEN("a simple class with a talent") {
    auto data = R"(
      <class name="Baby">
          <tree name="Boy">
              <tier>
                  <talent type="stats" name="Crawl"> <stats speed="2" /> </talent>
              </tier>
          </tree>
      </class>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a level-1 user tries to take that talent") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.levelUp();
      c.sendMessage(CL_TAKE_TALENT, "Crawl");

      THEN("he has has that talent") {
        const auto &classType = s.getFirstClass();
        const auto *talent = classType.findTalent("Crawl");
        WAIT_UNTIL(user.getClass().hasTalent(talent));
      }
    }
  }
}

TEST_CASE("A talent tier can require a tool") {
  GIVEN("a talent tier that requires a tool") {
    auto data = R"(
      <class name="Doctor">
          <tree name="Surgeon">
              <tier>
                  <requires tool="medicalSchool" />
                  <talent type="stats" name="Study"> <stats energy="1" /> </talent>
              </tier>
          </tree>
      </class>
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a level-1 user tries to take that talent") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.levelUp();
      c.sendMessage(CL_TAKE_TALENT, "Study");
      REPEAT_FOR_MS(100);

      THEN("he does not have that talent") {
        const auto &classType = s.getFirstClass();
        const auto *talent = classType.findTalent("Study");
        CHECK_FALSE(user.getClass().hasTalent(talent));
      }
    }
  }
}
