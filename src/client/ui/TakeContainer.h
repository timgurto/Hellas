#ifndef TAKE_CONTAINER_H
#define TAKE_CONTAINER_H

#include "../ClientItem.h"
#include "Element.h"

class List;

// An alternative to a container, that allows only taking items, and not
// swapping.  e.g., loot.
class TakeContainer : public Element {
 public:
  TakeContainer(ClientItem::vect_t &linked, size_t serial,
                const ScreenRect &rect);

  void repopulate();
  size_t size() const { return _list->size(); }

  // Send a CL_TAKE message.  data: a pair containing the serial and slot.
  static void take(void *data);

  static const size_t LOOT_CAPACITY = 8;

 private:
  ClientItem::vect_t &_linked;
  size_t _serial;  // The serial of the object with this container.

  List *_list;

  typedef std::pair<size_t, size_t> slot_t;
  std::vector<slot_t>
      _slots;  // slot -> serial/slot pairs, for button functions
};

#endif
