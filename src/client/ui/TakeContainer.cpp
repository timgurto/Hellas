#include "TakeContainer.h"

#include <cassert>
#include <utility>

#include "../../Message.h"
#include "../Client.h"
#include "Button.h"
#include "ColorBlock.h"
#include "ConfirmationWindow.h"
#include "Label.h"
#include "List.h"

TakeContainer::TakeContainer(ClientItem::vect_t &linked, Serial serial,
                             const ScreenRect &rect, Client &client)
    : _linked(linked),
      _serial(serial),
      _slots(LOOT_CAPACITY),
      Element(rect),
      _list(new List(rect, Element::ITEM_HEIGHT + 2)) {
  assert(_serial.isInitialised());
  setClient(client);

  addChild(_list);
  for (size_t i = 0; i != LOOT_CAPACITY; ++i)
    _slots[i] = std::make_pair(_serial, i);
}

TakeContainer *TakeContainer::CopyFrom(ClientItem::vect_t &linked,
                                       Serial serial, const ScreenRect &rect,
                                       Client &client) {
  return new TakeContainer(linked, serial, rect, client);
}

void TakeContainer::repopulate() {
  px_t oldScroll = _list->scrollPos();

  // Find the highest non-empty slot, to exclude all subsequent slots.
  // This will avoid trailing empty slots causing the window to scroll.
  bool thereIsAnyLoot = false;
  int lastNonEmptySlot = _linked.size() - 1;
  for (; lastNonEmptySlot >= 0; --lastNonEmptySlot)
    if (_linked[lastNonEmptySlot].first.type() != nullptr) {
      thereIsAnyLoot = true;
      break;
    }

  _list->clearChildren();
  if (!thereIsAnyLoot) return;

  for (int i = 0; i <= lastNonEmptySlot && i < LOOT_CAPACITY; ++i) {
    auto *row = new Element;
    _list->addChild(row);

    const auto &slot = _linked[i];
    const auto *itemType = slot.first.type();
    if (!itemType) continue;

    auto pSlot = &_slots[i];
    auto *button = new Button({0, 0, row->width(), row->height()}, {},
                              [this, pSlot]() { take(pSlot); });
    row->addChild(button);
    button->addChild(new Picture(1, 1, itemType->icon()));
    px_t labX = Client::ICON_SIZE + 2;
    button->addChild(new Label({labX, 0, row->width() - labX, row->height()},
                               itemType->name(), LEFT_JUSTIFIED,
                               CENTER_JUSTIFIED));
    const auto quantity = slot.second;
    if (quantity > 1)
      button->addChild(new Label({labX, 0, row->width() - labX, row->height()},
                                 "x"s + toString(quantity) + " ",
                                 RIGHT_JUSTIFIED, CENTER_JUSTIFIED));
  }

  _list->scrollPos(oldScroll);
}

void TakeContainer::take(void *data) {
  slot_t &slot = *reinterpret_cast<slot_t *>(data);
  auto *&window = _client->_confirmLootSoulboundItem;

  const auto windowText =
      "Looting this item will bind it to you.  Are you sure?";
  const auto msgArgs = makeArgs(slot.first, slot.second);
  if (window != nullptr) {
    _client->removeWindow(window);
    delete window;
  }
  window = new ConfirmationWindow(*_client, windowText, CL_TAKE_ITEM, msgArgs);
  _client->addWindow(window);
  window->show();
}
