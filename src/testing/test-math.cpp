#include <SDL.h>

#include "../Color.h"
#include "../HasTags.h"
#include "../NormalVariable.h"
#include "../Point.h"
#include "../server/Groups.h"
#include "../server/Server.h"
#include "TestFixtures.h"
#include "catch.hpp"
#include "testing.h"

TEST_CASE("Convert from Uint32 to Color and back") {
  Uint32 testNum = rand();
  Color c = testNum;
  Uint32 result = c;
  CHECK(testNum == result);
}

TEST_CASE("Zero-SD normal variables") {
  NormalVariable a(0, 0);
  CHECK(a.generate() == 0);
  CHECK(a() == 0);

  NormalVariable b(1, 0);
  CHECK(b.generate() == 1);

  NormalVariable c(-0.5, 0);
  CHECK(c.generate() == -0.5);
}

TEST_CASE("Complex normal variable") {
  NormalVariable default;
  NormalVariable v(10, 2);
  static const size_t SAMPLES = 1000;
  size_t numWithin1SD = 0, numWithin2SD = 0;
  for (size_t i = 0; i != SAMPLES; ++i) {
    double sample = v.generate();
    if (sample > 8 && sample < 12) ++numWithin1SD;
    if (sample > 6 && sample < 14) ++numWithin2SD;
  }
  double proportionWithin1SD = 1.0 * numWithin1SD / SAMPLES,
         proportionBetween1and2SD =
             1.0 * (numWithin2SD - numWithin1SD) / SAMPLES;
  // Proportion within 1 standard deviation should be around 0.68
  CHECK(proportionWithin1SD >= 0.63);
  CHECK(proportionWithin1SD <= 0.73);
  // Proportion between 1 and 2 standard deviations should be around 0.27
  CHECK(proportionBetween1and2SD >= 0.22);
  CHECK(proportionBetween1and2SD <= 0.32);
}

TEST_CASE("NormalVariable copying") {
  NormalVariable nv1;
  NormalVariable nv2 = nv1;
  nv1();
}

TEST_CASE("Distance-to-line with A=B") {
  MapPoint p(10, 10);
  MapPoint q(5, 3);
  distance(p, q, q);
}

TEST_CASE("getTileRect() behaves as expected") {
  CHECK(Map::getTileRect(0, 0) == MapRect(-16, 0, 32, 32));
  CHECK(Map::getTileRect(1, 0) == MapRect(16, 0, 32, 32));
  CHECK(Map::getTileRect(0, 1) == MapRect(0, 32, 32, 32));
  CHECK(Map::getTileRect(1, 1) == MapRect(32, 32, 32, 32));
  CHECK(Map::getTileRect(4, 0) == MapRect(112, 0, 32, 32));
  CHECK(Map::getTileRect(5, 0) == MapRect(144, 0, 32, 32));
  CHECK(Map::getTileRect(0, 4) == MapRect(-16, 128, 32, 32));
  CHECK(Map::getTileRect(0, 5) == MapRect(0, 160, 32, 32));
  CHECK(Map::getTileRect(7, 5) == MapRect(224, 160, 32, 32));
  CHECK(Map::getTileRect(7, 6) == MapRect(208, 192, 32, 32));
}

TEST_CASE("toPascal") {
  CHECK(toPascal("") == "");
  CHECK(toPascal("A") == "A");
  CHECK(toPascal("B") == "B");
  CHECK(toPascal("a") == "A");
  CHECK(toPascal("AA") == "Aa");
  CHECK(toPascal("AB") == "Ab");
  CHECK(toPascal("AAA") == "Aaa");
}

TEST_CASE("Tool-speed display", "[ui][tool]") {
  HasTags thing;
  thing.addTag("fast", 1.1);
  CHECK(thing.toolSpeedDisplayText("fast") == " +10%"s);
}

TEST_CASE_METHOD(TwoClients, "Roll command", "[grouping]") {
  WHEN("Alice rolls") {
    cAlice.sendMessage(CL_ROLL);

    THEN("She receives the result") {
      CHECK(cAlice.waitForMessage(SV_ROLL_RESULT));
    }

    THEN("Bob does not receive the result") {
      CHECK_FALSE(cBob.waitForMessage(SV_ROLL_RESULT));
    }
  }

  GIVEN("Alice and Bob are in a group") {
    server->groups->addToGroup("Alice", "Bob");

    WHEN("Alice rolls") {
      cAlice.sendMessage(CL_ROLL);

      THEN("Bob receives the result") {
        CHECK(cBob.waitForMessage(SV_ROLL_RESULT));
      }
    }
  }

  auto hit1 = false, hit100 = false;
  for (auto i = 0; i < 10000; ++i) {
    auto result = roll();
    CHECK(result >= 1);
    CHECK(result <= 100);
    if (result == 1)
      hit1 = true;
    else if (result == 100)
      hit100 = true;
  }
  CHECK(hit1);
  CHECK(hit100);
}
