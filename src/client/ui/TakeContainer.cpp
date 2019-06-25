#include "TakeContainer.h"

#include <cassert>
#include <utility>

#include "../Client.h"
#include "Button.h"
#include "ColorBlock.h"
#include "Label.h"
#include "List.h"

TakeContainer::TakeContainer(ClientItem::vect_t &linked, size_t serial,
                             const ScreenRect &rect)
    : _linked(linked),
      _serial(serial),
      _slots(LOOT_CAPACITY),
      Element(rect),
      _list(new List(rect, Element::ITEM_HEIGHT + 2)) {
  assert(_serial != 0);

  addChild(_list);
  for (size_t i = 0; i != LOOT_CAPACITY; ++i)
    _slots[i] = std::make_pair(_serial, i);
}

void TakeContainer::repopulate() {
  px_t oldScroll = _list->scrollPos();

  // Find the highest non-empty slot, to exclude all subsequent slots
  int lastNonEmptySlot = _linked.size() - 1;
  for (; lastNonEmptySlot >= 0; --lastNonEmptySlot)
    if (_linked[lastNonEmptySlot].first.type() != nullptr) break;

  _list->clearChildren();
  if (lastNonEmptySlot == -1) return;

  for (int i = 0; i <= lastNonEmptySlot; ++i) {
    auto &slot = _linked[i];
    const auto *itemType = slot.first.type();
    Element *dummy = new Element;
    _list->addChild(dummy);
    if (itemType != nullptr) {
      auto pSlot = &_slots[i];
      Button *button = new Button({0, 0, dummy->width(), dummy->height()}, "",
                                  [pSlot]() { take(pSlot); });
      dummy->addChild(button);
      button->addChild(new Picture(1, 1, itemType->icon()));
      px_t labX = Client::ICON_SIZE + 2;
      button->addChild(
          new Label({labX, 0, dummy->width() - labX, dummy->height()},
                    itemType->name(), LEFT_JUSTIFIED, CENTER_JUSTIFIED));
      if (slot.second > 1)
        button->addChild(
            new Label({labX, 0, dummy->width() - labX, dummy->height()},
                      std::string("x") + toString(slot.second) + " ",
                      RIGHT_JUSTIFIED, CENTER_JUSTIFIED));
    }
  }

  _list->scrollPos(oldScroll);
}

void TakeContainer::take(void *data) {
  slot_t &slot = *reinterpret_cast<slot_t *>(data);
  Client::_instance->sendMessage(CL_TAKE_ITEM,
                                 makeArgs(slot.first, slot.second));
}
