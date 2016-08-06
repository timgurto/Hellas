// (C) 2016 Tim Gurto

#ifndef TESTING_H
#define TESTING_H

#include <string>
#include <vector>

struct Test{
    std::string description;
    typedef bool (*testFun_t)();
    testFun_t fun;
    
    typedef std::vector<Test> testContainer_t;
    static testContainer_t testContainer;

    Test(std::string &description, testFun_t fun);
};

void addTest(const char *description, Test::testFun_t fun);

#endif
