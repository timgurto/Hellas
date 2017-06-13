#ifndef TEST_H
#define TEST_H

#include <string>
#include <vector>

#include "../Args.h"
#include "../types.h"

class Test{
public:
    typedef bool (*testFun_t)();
    typedef std::vector<Test> testContainer_t;

private:
    std::string _description;

    testFun_t _fun;
    bool _slow;
    bool _quarantined;
    
    static testContainer_t *_testContainer;
    static bool _runSubset; // At least one test was defined with ONLY_TEST

public:
    static testContainer_t &testContainer();
    static testContainer_t &testSubset();

    static const bool
        SLOW = true,
        FAST = false,
        IN_SUBSET = true,
        NOT_IN_SUBSET = false,
        QUARANTINED = true,
        NOT_QUARANTINED = false;

    Test(std::string description, bool slow, bool subset, bool quarantined, testFun_t fun);

    const std::string &description() const { return _description; }
    testFun_t fun() const { return _fun; }
    bool shouldSkip() const;

    static void signalThrower(int signal); // A signal handler that throws the signal.
};

#define TOKEN_CONCAT_2(a, b) a ## b
#define TOKEN_CONCAT(a, b) TOKEN_CONCAT_2(a, b)

#define TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, \
            Test::FAST, \
            Test::NOT_IN_SUBSET, \
            Test::NOT_QUARANTINED, \
            [](){
#define SLOW_TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, \
            Test::SLOW, \
            Test::NOT_IN_SUBSET, \
            Test::NOT_QUARANTINED, \
            [](){
#define ONLY_TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, \
            Test::FAST, \
            Test::IN_SUBSET, \
            Test::NOT_QUARANTINED, \
            [](){
#define QUARANTINED_TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, \
            Test::FAST, \
            Test::NOT_IN_SUBSET, \
            Test::QUARANTINED, \
            [](){

#define TEND return true; });

#define REPEAT_FOR_MS(TIME_TO_REPEAT) \
        for (ms_t startTime = SDL_GetTicks(); SDL_GetTicks() < startTime + (TIME_TO_REPEAT); )
#define WAIT_UNTIL_TIMEOUT(x, TIMEOUT) \
        do { \
            bool _success = false; \
            for (ms_t startTime = SDL_GetTicks(); SDL_GetTicks() < startTime + (TIMEOUT); ) { \
                _success = x; \
                if (_success) \
                    break; \
            } \
            if (! _success ) { \
                return false; \
            } \
        } while (0)
#define DEFAULT_TIMEOUT (3000)
#define WAIT_UNTIL(x) WAIT_UNTIL_TIMEOUT((x), (DEFAULT_TIMEOUT))
#define WAIT_FOREVER_UNTIL(x) while( ! (x) )

#define ENSURE(x) do {if ( ! (x) ) { return false; } } while (false)

#endif
