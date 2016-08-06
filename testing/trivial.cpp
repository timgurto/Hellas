// (C) 2016 Tim Gurto

#include "testing.h"

void trivialTests(){
    testContainer.push_back([](){ return 1+1==2; });
    testContainer.push_back([](){ return 1+1!=2; });
}
