#include <cassert>
#include <utility>

#include "Button.h"
#include "ColorBlock.h"
#include "Label.h"
#include "List.h"
#include "TakeContainer.h"
#include "../Client.h"

TakeContainer::TakeContainer(ClientItem::vect_t &linked, size_t serial, const Rect &rect):
_linked(linked),
_serial(serial),
Element(rect),
_list(new List(rect, Element::ITEM_HEIGHT + 2)){
    assert(_serial != 0);

    addChild(_list);
    for (size_t i = 0; i != _linked.size(); ++i)
        _slots[i] = std::make_pair(_serial, i);
}

void TakeContainer::repopulate(){
    px_t oldScroll = _list->scrollPos();

    // Find the highest non-empty slot, to exclude all subsequent slots
    int lastNonEmptySlot = _linked.size() - 1;
    for (; lastNonEmptySlot >= 0; --lastNonEmptySlot)
        if (_linked[lastNonEmptySlot].first != nullptr)
            break;

    if (lastNonEmptySlot == -1)
        return;

    _list->clearChildren();
    for (int i = 0; i <= lastNonEmptySlot; ++i) {
        auto &slot = _linked[i];
        Element *dummy = new Element;
        _list->addChild(dummy);
        if (slot.first != nullptr){
            Button *button = new Button(Rect(0, 0, dummy->width(), dummy->height()), "",
                                        take, &_slots[i]);
            dummy->addChild(button);
            button->addChild(new Picture(1, 1, slot.first->icon()));
            px_t labX = Client::ICON_SIZE + 2;
            button->addChild(new Label(Rect(labX, 0, dummy->width() - labX, dummy->height()),
                                       slot.first->name(), LEFT_JUSTIFIED, CENTER_JUSTIFIED));
            if (slot.second > 1)
                button->addChild(new Label(Rect(labX, 0, dummy->width() - labX, dummy->height()),
                                           std::string("x") + toString(slot.second) + " ",
                                           RIGHT_JUSTIFIED, CENTER_JUSTIFIED));
        }
    }

    _list->scrollPos(oldScroll);
}

void TakeContainer::take(void *data){
    slot_t &slot = *reinterpret_cast<slot_t *>(data);
    Client::_instance->sendMessage(CL_TAKE_ITEM, makeArgs(slot.first, slot.second));
}
