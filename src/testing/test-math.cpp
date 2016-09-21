// (C) 2016 Tim Gurto

#include <csignal>
#include <cstdlib>
#include <SDL.h>

#include "Test.h"
#include "../Color.h"
#include "../NormalVariable.h"

TEST("Convert from Uint32 to Color and back")
    Uint32 testNum = rand();
    Color c = testNum;
    Uint32 result = c;
    return testNum == result;
TEND

TEST("Zero-SD normal variables")
    NormalVariable
        a(0, 0),
        b(1, 0),
        c(-0.5, 0);
    return
        a.generate() == 0 &&
        b.generate() == 1 &&
        c.generate() == -0.5;
TEND

TEST("Complex normal variable")
    NormalVariable v(10, 2);
    static const size_t SAMPLES = 1000;
    size_t
        numWithin1SD = 0,
        numWithin2SD = 0;
    for (size_t i = 0; i != SAMPLES; ++i){
        double sample = v.generate();
        if (sample > 8 && sample < 12)
            ++numWithin1SD;
        if (sample > 6 && sample < 14)
            ++numWithin2SD;
    }
    double
        proportionWithin1SD = 1.0 * numWithin1SD / SAMPLES,
        proportionBetween1and2SD = 1.0 * (numWithin2SD - numWithin1SD) / SAMPLES;
    // Proportion within 1 standard deviation should be between 0.58 and 0.78
    if (proportionWithin1SD < 0.58 || proportionWithin1SD > 0.78)
        return false;
    // Proportion between 1 and 2 standard deviations should be between 0.17 and 0.37
    if (proportionBetween1and2SD < 0.17 || proportionBetween1and2SD > 0.37)
        return false;
    return true;
TEND

TEST("NormalVariable copying")
    bool ret = true;
    
    auto prevHandler = std::signal(SIGSEGV, Test::signalThrower); // Set signal handler
    int prevAssertMode = _CrtSetReportMode(_CRT_ASSERT,0); // Disable asserts

    try {
        NormalVariable nv1;
        NormalVariable nv2 = nv1;
        nv1();
    } catch(int) {
        ret = false;
    }

    _CrtSetReportMode(_CRT_ASSERT,prevAssertMode); // Re-enable asserts
    std::signal(SIGSEGV, prevHandler); // Reset signal handler

    return ret;
TEND
