// (C) 2016 Tim Gurto

#include "Test.h"
#include "../curlUtil.h"

TEST("Read invalid URL")
    return readFromURL("fake.fake.fake").empty();
TEND

TEST("Read badly-formed URL")
    return readFromURL("1").empty();
TEND

TEST("Read blank URL")
    return readFromURL("").empty();
TEND

TEST("Read test URL")
    return readFromURL("timgurto.com/test.txt") ==
            "This is a file for testing web-access accuracy.";
TEND
