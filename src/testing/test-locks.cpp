#include "TemporaryUserStats.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Unlock-chance-bonus stat", "[.slow][unlocking]") {
  GIVEN("obtaining a fish has a 50% chance of teaching how to catch a fish") {
    auto data = R"(
      <item id="fish" />
      <recipe id="catchAFish" product="fish" >
        <unlockedBy item="fish" chance="0.5" />
      </recipe>
    )";
    auto server = TestServer::WithDataString(data);

    AND_GIVEN("users have a bonus chance of +100%") {
      CHANGE_BASE_USER_STATS.unlockBonus(10000);

      for (auto i = 0; i != 40; ++i) {
        // Give the user a fish
        auto client = TestClient::WithDataString(data);
        server.waitForUsers(1);
        auto &user = server.getFirstUser();
        const auto &fish = server.getFirstItem();
        user.giveItem(&fish);

        // Make sure he knows a recipe
        REQUIRE(user.knownRecipes().size() == 1);
      }
    }

    AND_GIVEN("users have a bonus chance of +50%") {
      CHANGE_BASE_USER_STATS.unlockBonus(5000);

      THEN("75% of unlock attempts succeed") {
        auto numSuccessfulUnlocks = 0;
        for (auto i = 0; i != 100; ++i) {
          // Give user a fish
          auto client = TestClient::WithDataString(data);
          server.waitForUsers(1);
          auto &user = server.getFirstUser();
          const auto &fish = server.getFirstItem();
          user.giveItem(&fish);

          // Check whether he now knows a recipe") {
          if (user.knownRecipes().size() == 1) ++numSuccessfulUnlocks;
        }

        CHECK_ROUGHLY_EQUAL(numSuccessfulUnlocks, 75, 0.2);
      }
    }
  }
}

TEST_CASE("The locking systems", "[unlocking][crafting]") {
  GIVEN("obtaining an item has a 0% chance of unlocking a pencil recipe") {
    auto data = R"(
      <item id="pencil" />
      <recipe id="pencil" >
        <unlockedBy item="pencil" chance="0" />
      </recipe>
    )";
    auto server = TestServer::WithDataString(data);

    WHEN("a user obtains an item") {
      auto client = TestClient::WithDataString(data);
      server.waitForUsers(1);
      auto &user = server.getFirstUser();
      const auto &item = server.getFirstItem();
      user.giveItem(&item);

      THEN("he doesn't know any recipes") {
        REQUIRE(user.knownRecipes().size() == 0);
      }
    }
  }
}
