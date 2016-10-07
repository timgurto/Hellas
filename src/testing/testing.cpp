// (C) 2016 Tim Gurto

#include <iomanip>
#include <iostream>

#include <SDL.h>
#include "Test.h"
#include "../client/Renderer.h"
#include "../Args.h"
#include "../Socket.h"
#include "../server/LogConsole.h"

Args cmdLineArgs;
Renderer renderer;

int main(int argc, char **argv){
    srand(static_cast<unsigned>(time(0)));

    renderer.init();

    cmdLineArgs.init(argc, argv);

    LogConsole log;
    Socket::debug = &log;
    if (cmdLineArgs.contains("quiet"))
        log.quiet();

    std::string
        colorStart = "\x1B[38;2;0;127;127m",
        colorFail  = "\x1B[38;2;255;0;0m",
        colorPass  = "\x1B[38;2;0;192;0m",
        colorSkip  = "\x1B[38;2;51;51;51m",
        colorReset = "\x1B[0m";

    size_t
        i = 0,
        passed = 0,
        failed = 0,
        skipped = 0;
    for (const Test &test : Test::testContainer()){
        ++i;
        const std::string *statusColor;
        size_t gapLength = Test::STATUS_MARGIN - 
                           min(Test::STATUS_MARGIN, test.description().length());
        std::string gap(gapLength, ' ');
        std::cout
            << colorStart << std::setw(4) << i << colorReset << ' '
            << test.description() << gap << std::flush;
        if (test.shouldSkip()) {
            statusColor = &colorSkip;
            ++skipped;
        } else {
            bool result = test.fun()();
            if (!result){
                statusColor = &colorFail;
                ++failed;
            } else {
                statusColor = &colorPass;
                ++passed;
            }
        }
        std::cout << *statusColor << char(254) << colorReset << std::endl;
    }

    std::cout << "----------" << std::endl
              << "  " << i << " tests in total";
    if (passed > 0)  std::cout << ", " << colorPass << passed  << " passed"  << colorReset;
    if (failed > 0)  std::cout << ", " << colorFail << failed  << " failed"  << colorReset;
    if (skipped > 0) std::cout << ", " << colorSkip << skipped << " skipped" << colorReset;
    std::cout << "." << std::endl;

    return 0;
}
