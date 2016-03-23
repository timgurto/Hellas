// (C) 2016 Tim Gurto

#include "ItemSelector.h"

ItemSelector::ItemSelector(int x, int y):
Button(Rect(x, y, Element::ITEM_HEIGHT, Element::ITEM_HEIGHT), ""
       ),
_item(0)
{}
