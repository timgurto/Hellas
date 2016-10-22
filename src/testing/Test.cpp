// (C) 2016 Tim Gurto

#include <iostream>

#include "Test.h"
#include "../Args.h"

extern Args cmdLineArgs;

Test::testContainer_t *Test::_testContainer = nullptr;
bool Test::_runSubset = false;
const size_t Test::STATUS_MARGIN = 45;

Test::Test(std::string description, bool slow, bool subset, testFun_t fun):
_description(description),
_fun(fun),
_slow(slow){
    if (subset) {
        if (!_runSubset)
            testContainer().clear();
        _runSubset = true;
        testContainer().push_back(*this);
    }
    
    else if (!_runSubset)
        testContainer().push_back(*this);
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
    return _slow && cmdLineArgs.contains("skipSlow");
}
