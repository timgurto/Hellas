// (C) 2016 Tim Gurto

#include <iostream>
#include "Test.h"

Test::testContainer_t *Test::_testContainer = nullptr;
Args Test::args;
const size_t Test::STATUS_MARGIN = 50;

Test::Test(std::string description, bool slow, testFun_t fun):
    _description(description),
    _fun(fun),
    _slow(slow){
    Test::testContainer().push_back(*this);
}

Test::testContainer_t &Test::testContainer(){
    if (_testContainer == nullptr)
        _testContainer = new testContainer_t;
    return *_testContainer;
}

void Test::signalThrower(int signal){
    throw signal;
}

bool Test::shouldSkip() const{
    return _slow && args.contains("skipSlow");
}
