#include "TestServer.h"
#include "TestClient.h"
#include "testing.h"

TEST_CASE("Leveling up restores health and energy") {
    auto s = TestServer{};
    auto c = TestClient{};

    // Given a damaged user
    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    user.reduceHealth(1);
    CHECK(user.health() < user.stats().maxHealth);

    // When the user levels up

    // Then the user's health and energy are full
    WAIT_UNTIL(user.health() == user.stats().maxHealth);
}
