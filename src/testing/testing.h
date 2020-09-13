#ifndef TESTING_H
#define TESTING_H

#undef min
#undef max
#include "catch.hpp"

#define REPEAT_FOR_MS(TIME_TO_REPEAT)   \
  for (ms_t startTime = SDL_GetTicks(); \
       SDL_GetTicks() < startTime + (TIME_TO_REPEAT);)
#define WAIT_UNTIL_TIMEOUT(x, TIMEOUT) \
  do {                                 \
    REPEAT_FOR_MS(TIMEOUT) {           \
      if (x) break;                    \
    }                                  \
    REQUIRE(x);                        \
  } while (0)
#define DEFAULT_TIMEOUT (3000)
#define WAIT_UNTIL(x) WAIT_UNTIL_TIMEOUT((x), (DEFAULT_TIMEOUT))

#define CHECK_ROUGHLY_EQUAL(A, B, EPSILON_PROPORTION) \
  {                                                   \
    auto epsilon = (EPSILON_PROPORTION) * (B);        \
    INFO(#A " = " << (A));                            \
    INFO(#B " = " << (B));                            \
    INFO("Allowable difference = " << epsilon);       \
    auto difference = abs((A) - (B));                 \
    CHECK(difference < epsilon);                      \
  }

#endif
