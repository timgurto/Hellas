// (C) 2016 Tim Gurto

#ifndef TESTING_H
#define TESTING_H

#include <vector>

typedef bool (*testFun_t)();
typedef std::vector<testFun_t> testContainer_t;

extern testContainer_t testContainer;

#endif
