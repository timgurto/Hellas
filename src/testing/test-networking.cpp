#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
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

SLOW_TEST("Use socket after cleanup")
    bool ret = true;
    // Run twice: Winsock will be cleaned up after the first iteration.
    for (size_t i = 0; i != 2; ++i){
        TestServer s;
        TestClient c;
        s.run();
        c.run();

        ret = c.connected(); // false in case of a connection error.
    }
    return ret;
TEND
