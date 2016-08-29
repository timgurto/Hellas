// (C) 2016 Tim Gurto

#ifndef TEST_H
#define TEST_H

#include <string>
#include <vector>

class Test{
public:
    typedef bool (*testFun_t)();
    typedef std::vector<Test> testContainer_t;

private:
    std::string _description;
    testFun_t _fun;
    
    static testContainer_t *_testContainer;

public:
    static testContainer_t &testContainer();
    Test(std::string description, testFun_t fun);

    const std::string &description() const { return _description; }
    testFun_t fun() const { return _fun; }
};

#define TOKEN_CONCAT_2(a, b) a ## b
#define TOKEN_CONCAT(a, b) TOKEN_CONCAT_2(a, b)
#define TEST(name) static Test TOKEN_CONCAT(test_, __COUNTER__) (name, [](){
#define TEND });

#endif
