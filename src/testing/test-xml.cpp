// (C) 2016 Tim Gurto

#include "Test.h"
#include "../XmlReader.h"

TEST("Read XML file with root only")
    XmlReader xr("testing/empty.xml");
    for (auto elem : xr.getChildren("nonexistent_tag"))
        ;
    return true;
TEND
