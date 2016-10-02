// (C) 2016 Tim Gurto

#include <csignal>
#include <cstdlib>

#include "Test.h"
#include "../client/Texture.h"

TEST("Use texture after removing all others")
    static const std::string IMAGE_FILE = "testing/tiny.png";
    {
        Texture t(IMAGE_FILE);
    }

    auto prevHandler = std::signal(SIGSEGV, Test::signalThrower); // Set signal handler
    int prevAssertMode = _CrtSetReportMode(_CRT_ASSERT,0); // Disable asserts

    bool ret = true;
    try {
        Texture t(IMAGE_FILE);
    } catch(int) {
        ret = false;
    }

    _CrtSetReportMode(_CRT_ASSERT,prevAssertMode); // Re-enable asserts
    std::signal(SIGSEGV, prevHandler); // Reset signal handler
    return ret;
TEND
