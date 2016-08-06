// (C) 2016 Tim Gurto

#include "testing.h"

void trivialTests(){
    addTest("Basic pass case", [](){ return 1+1==2; });
    addTest("Basic fail case", [](){ return 1+1!=2; });
}
