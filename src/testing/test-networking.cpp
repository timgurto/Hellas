// (C) 2016 Tim Gurto

#include <thread>

#include "Test.h"
#include "../Socket.h"
#include "../curlUtil.h"
#include "../client/Client.h"
#include "../server/Server.h"

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
        Server s;
        std::thread([& s](){ s.run(); }).detach(); // Run server loop in new thread

        //Create client and wait for login
        Client c;
        std::thread([& c](){ c.run(); }).detach(); // Run client loop in new thread
        while (c._connectionStatus == Client::TRYING) ;
        if (c._connectionStatus == Client::CONNECTED)
            ret = true;
        else
            ret = false;

        //Kill client and server
        c._loop = false;
        s._loop = false;
        while (c._running) ;
        while (s._running) ;
    }
    return ret;
TEND
