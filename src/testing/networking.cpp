// (C) 2016 Tim Gurto

#include "testing.h"
#include "../curlUtil.h"

void networkingTests(){
    addTest("Read invalid URL", [](){
        return readFromURL("fake.fake.fake").empty();
    });
    addTest("Read badly formed URL", [](){
        return readFromURL("1").empty();
    });
    addTest("Read blank URL", [](){
        return readFromURL("").empty();
    });
    addTest("Read test URL", [](){
        return readFromURL("timgurto.com/test.txt") ==
            "This is a file for testing web-access accuracy.";
    });
}
