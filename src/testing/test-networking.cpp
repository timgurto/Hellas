// (C) 2016 Tim Gurto

#include "ClientTestInterface.h"
#include "ServerTestInterface.h"
#include "Test.h"
#include "../Socket.h"
#include "../curlUtil.h"

SLOW_TEST("Read invalid URL")
    return readFromURL("fake.fake.fake").empty();
TEND

SLOW_TEST("Read badly-formed URL")
    return readFromURL("1").empty();
TEND

TEST("Read blank URL")
    return readFromURL("").empty();
TEND

TEST("Read test URL")
    return readFromURL("timgurto.com/test.txt") ==
            "This is a file for testing web-access accuracy.";
TEND

TEST("Use socket after cleanup")
    bool ret = true;
    // Run twice: Winsock will be cleaned up after the first iteration.
    for (size_t i = 0; i != 2; ++i){
        ServerTestInterface s;
        ClientTestInterface c;
        s.run();
        c.run();

        ret = c.connected(); // false in case of a connection error.

        c.stop();
        s.stop();
    }
    return ret;
TEND
