// (C) 2016 Tim Gurto

#include <iostream>

#include <SDL.h>
#include "Test.h"
#include "../client/Renderer.h"
#include "../Args.h"

Args cmdLineArgs;
Renderer renderer;

int main(int argc, char **argv){
    renderer.init();

    Test::args.init(argc, argv);

    size_t failed = 0;
    size_t i = 0;
    size_t skipped = 0;
    for (const Test &test : Test::testContainer()){
        ++i;
        if (test.shouldSkip()) {
            ++skipped;
            continue;
        }
        std::cout << "  " << test.description() << "... " << std::flush;
        bool result = test.fun()();
        if (!result){
            std::cout << "\x1b[31mFAIL\x1B[0m";
            ++failed;
        }
        std::cout << std::endl;
    }

    std::cout << "----------" << std::endl
              << "  " << i << " tests in total";
    size_t passed = i - failed - skipped;
    if (passed > 0) std::cout << ", " << passed << " passed";
    if (failed > 0) std::cout << ", " << failed << " failed";
    if (skipped > 0) std::cout << ", " << skipped << " skipped";
    std::cout << "." << std::endl;
    return 0;
}
