// (C) 2016 Tim Gurto

#include <iostream>
#include "testing.h"
#include "tests.h"

Test::testContainer_t Test::testContainer;

Test::Test(std::string &description, testFun_t fun):
    description(description),
    fun(fun){}

void addTest(const char *description, Test::testFun_t fun){
    Test::testContainer.push_back(Test(std::string(description), fun));
}

int main(){
    populate();

    size_t failures = 0;
    size_t i = 0;
    for (const Test &test : Test::testContainer){
        ++i;
        std::cout << "  " << test.description << "... " << std::flush;
        bool result = test.fun();
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
}
