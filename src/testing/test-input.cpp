#include "TestClient.h"
#include "testing.h"

TEST_CASE("KeyboardStateFetcher unit tests") {
  GIVEN("a client") {
    auto c = TestClient{};

    THEN("the client thinks 'a' is not pressed") {
      CHECK_FALSE(c.isKeyPressed(SDL_SCANCODE_A));

      AND_WHEN("the 'a' key is faked") {
        c.simulateKeypress(SDL_SCANCODE_A);

        THEN("the client think the 'b' key is not pressed") {
          CHECK_FALSE(c.isKeyPressed(SDL_SCANCODE_B));

          AND_WHEN("the 'b' key is faked") {
            c.simulateKeypress(SDL_SCANCODE_B);

            THEN("the client thinks 'a' and 'b' are pressed") {
              CHECK(c.isKeyPressed(SDL_SCANCODE_A));
              CHECK(c.isKeyPressed(SDL_SCANCODE_B));

              AND_THEN("the client thinks other keys are not pressed") {
                CHECK_FALSE(c.isKeyPressed(SDL_SCANCODE_M));
              }
            }
          }
        }
      }
    }
  }
}
