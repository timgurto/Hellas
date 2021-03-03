#ifndef TAKE_CONTAINER_H
#define TAKE_CONTAINER_H

#include "../../Serial.h"
#include "../ClientItem.h"
#include "Element.h"
#include "List.h"

class ConfirmationWindow;

// An alternative to a container, that allows only taking items, and not
// swapping.  e.g., loot.
class TakeContainer : public Element {
 public:
  static TakeContainer *CopyFrom(ClientItem::vect_t &linked, Serial serial,
                                 const ScreenRect &rect, Client &client);

  void repopulate();
  size_t size() const { return _list->size(); }

  // Send a CL_TAKE message.  data: a pair containing the serial and slot.
  void take(void *data);

  static const size_t LOOT_CAPACITY = 10;  // TODO find a better solution

 private:
  TakeContainer(ClientItem::vect_t &linked, Serial serial,
                const ScreenRect &rect, Client &client);

  ClientItem::vect_t &_linked;
  Serial _serial;  // The serial of the object with this container.

  List *_list;

  typedef std::pair<Serial, size_t> slot_t;
  std::vector<slot_t>
      _slots;  // slot -> serial/slot pairs, for button functions

  void takeOnConfirmation();
};

#endif
