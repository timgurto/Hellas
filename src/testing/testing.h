#ifndef TESTING_H
#define TESTING_H

#include "catch.hpp"

#define REPEAT_FOR_MS(TIME_TO_REPEAT) \
        for (ms_t startTime = SDL_GetTicks(); SDL_GetTicks() < startTime + (TIME_TO_REPEAT); )
#define WAIT_UNTIL_TIMEOUT(x, TIMEOUT) \
        do { \
            bool _success = false; \
            for (ms_t startTime = SDL_GetTicks(); SDL_GetTicks() < startTime + (TIMEOUT); ) { \
                _success = x; \
                if (_success) \
                    break; \
            } \
            REQUIRE (x); \
        } while (0)
#define DEFAULT_TIMEOUT (3000)
#define WAIT_UNTIL(x) WAIT_UNTIL_TIMEOUT((x), (DEFAULT_TIMEOUT))

#endif
