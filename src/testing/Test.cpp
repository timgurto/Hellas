// (C) 2016 Tim Gurto

#include <iostream>
#include "Test.h"

Test::testContainer_t *Test::_testContainer = nullptr;

Test::Test(std::string description, testFun_t fun):
    _description(description),
    _fun(fun){
    Test::testContainer().push_back(*this);
    testContainer;
}

Test::testContainer_t &Test::testContainer(){
    if (_testContainer == nullptr)
        _testContainer = new testContainer_t;
    return *_testContainer;
}
