// (C) 2016 Tim Gurto

#ifndef TEST_H
#define TEST_H

#include <string>
#include <vector>

#include "../Args.h"

class Test{
public:
    typedef bool (*testFun_t)();
    typedef std::vector<Test> testContainer_t;

private:
    std::string _description;
    testFun_t _fun;
    bool _slow;
    
    static testContainer_t *_testContainer;
    static bool _runSubset; // At least one test was defined with ONLY_TEST

public:
    static testContainer_t &testContainer();
    static testContainer_t &testSubset();
    static const size_t STATUS_MARGIN;

    Test(std::string description, bool slow, bool subset, testFun_t fun);

    const std::string &description() const { return _description; }
    testFun_t fun() const { return _fun; }
    bool shouldSkip() const;

    static void signalThrower(int signal); // A signal handler that throws the signal.
};

#define TOKEN_CONCAT_2(a, b) a ## b
#define TOKEN_CONCAT(a, b) TOKEN_CONCAT_2(a, b)
#define TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, false, false, [](){
#define SLOW_TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, true, false, [](){
#define ONLY_TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, false, true, [](){
#define TEND });

#define WAIT_UNTIL(x) while( ! (x) )

#endif
