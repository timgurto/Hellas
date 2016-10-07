// (C) 2016 Tim Gurto

#include "Test.h"
#include "../client/ui/List.h"
#include "../client/ui/Label.h"

TEST("List size")
    List l(Rect(0, 0, 100, 100));
    if (l.size() != 0)
        return false;
    l.addChild(new Label(Rect(), "asdf"));
    return l.size() == 1;
TEND
