// (C) 2016 Tim Gurto

#include <iostream>

#include <SDL.h>
#include "Test.h"
#include "../client/Renderer.h"
#include "../Args.h"

Args cmdLineArgs;
Renderer renderer;

int main(int argc, char **argv){
    size_t failures = 0;
    size_t i = 0;
    for (const Test &test : Test::testContainer()){
        ++i;
        std::cout << "  " << test.description() << "... " << std::flush;
        bool result = test.fun()();
        if (!result){
            std::cout << "\x1b[31mFAIL\x1B[0m";
            ++failures;
        }
        std::cout << std::endl;
    }

    std::cout << "----------" << std::endl
              << "Ran " << i << " tests, "
              << i - failures << " passed, "
              << failures << " failed."
              << std::endl;
    return 0;
}
