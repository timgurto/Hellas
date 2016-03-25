// (C) 2016 Tim Gurto

#include "ItemSelector.h"

ItemSelector::ItemSelector(px_t x, px_t y):
Button(Rect(x, y, Element::ITEM_HEIGHT, Element::ITEM_HEIGHT), ""
       ),
_item(nullptr)
{}
