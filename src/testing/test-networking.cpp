#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"
#include "../Socket.h"
#include "../curlUtil.h"

TEST_CASE("Read invalid URL", "[.slow]"){
    CHECK(readFromURL("fake.fake.fake").empty());
}

TEST_CASE("Read badly-formed URL", "[.slow]"){
    CHECK(readFromURL("1").empty());
}

TEST_CASE("Read blank URL"){
    CHECK(readFromURL("").empty());
}

TEST_CASE("Read test URL", "[.slow]"){
    CHECK(readFromURL("timgurto.com/test.txt") ==
            "This is a file for testing web-access accuracy.");
}

TEST_CASE("Use socket after cleanup"){
    bool ret = true;
    // Run twice: Winsock will be cleaned up after the first iteration.
    for (size_t i = 0; i != 2; ++i){
        TestServer s;
        TestClient c;

        WAIT_UNTIL(c.connected()); // false in case of a connection error.
    }
}
