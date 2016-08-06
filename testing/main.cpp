// (C) 2016 Tim Gurto

#include <iostream>
#include "testing.h"
#include "tests.h"

testContainer_t testContainer;

int main(){
    populate();

    size_t failures = 0;
    size_t i = 0;
    for (testFun_t testFun : testContainer){
        ++i;
        std::cout << i << " ";
        bool result = testFun();
        if (result)
            std::cout << "pass";
        else {
            std::cout << "FAIL";
            ++failures;
        }
        std::cout << std::endl;
    }

    std::cout << "Ran " << i << " tests, "
              << i - failures << " passed, "
              << failures << " failed."
              << std::endl;
}