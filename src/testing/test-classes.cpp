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

TEST_CASE("A talent tier can require a tool") {
  GIVEN("a level-2 user, and talent tiers with various requirements") {
    auto data = R"(
      <class name="Doctor">
          <tree name="Surgeon">
              <tier>
                  <requires />
                  <talent type="stats" name="Meditate"> <stats energy="1" /> </talent>
              </tier>
              <tier>
                  <requires tool="medicalSchool" />
                  <talent type="stats" name="Study"> <stats energy="1" /> </talent>
              </tier>
          </tree>
      </class>
    )";
    auto s = TestServer::WithDataString(data);
    const auto &doctor = s.getFirstClass();
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.levelUp();

    WHEN("he tries to take the simple talent") {
      c.sendMessage(CL_TAKE_TALENT, "Meditate");

      THEN("he has it") {
        const auto *talent = doctor.findTalent("Meditate");
        WAIT_UNTIL(user.getClass().hasTalent(talent));
      }
    }

    WHEN("he tries to take the talent with a tool requirement") {
      c.sendMessage(CL_TAKE_TALENT, "Study");
      REPEAT_FOR_MS(100);

      THEN("he doesn't have it") {
        const auto *talent = doctor.findTalent("Study");
        CHECK_FALSE(user.getClass().hasTalent(talent));
      }
    }
  }
}
