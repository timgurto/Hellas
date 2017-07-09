#ifndef TAKE_CONTAINER_H
#define TAKE_CONTAINER_H

#include "Element.h"
#include "../ClientItem.h"

class List;

// An alternative to a container, that allows only taking items, and not swapping.  e.g., loot.
class TakeContainer : public Element{
public:
    TakeContainer(ClientItem::vect_t &linked, size_t serial, const Rect &rect);

    void repopulate();
    size_t size() const { return _list->size(); }

    // Send a CL_TAKE message.  data: a pair containing the serial and slot.
    static void take(void *data);

private:
    ClientItem::vect_t &_linked;
    size_t _serial; // The serial of the object with this container.

    List *_list;

    typedef std::pair<size_t, size_t> slot_t;
    std::map<size_t, slot_t> _slots; // slot -> serial/slot pairs, for button functions
};

#endif
