#include "Test.h"
#include "../Args.h"

extern Args cmdLineArgs;

Test::testContainer_t *Test::_testContainer = nullptr;
bool Test::_runSubset = false;
const size_t Test::STATUS_MARGIN = 50;

Test::Test(std::string description, bool slow, bool subset, bool quarantined, testFun_t fun):
_description(description),
_fun(fun),
_slow(slow),
_quarantined(quarantined){
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
    if (_quarantined)
        return true;
    if (_slow && cmdLineArgs.contains("skipSlow"))
        return true;
    return false;
}
