// (C) 2015 Tim Gurto

#include "ColorBlock.h"
#include "Container.h"
#include "../Client.h"
#include "../Renderer.h"
#include "../../server/User.h"

extern Renderer renderer;

const int Container::GAP = 0;

// TODO: set to Client::INVENTORY_SIZE.
const size_t Container::NO_SLOT = 999;

size_t Container::dragSlot = NO_SLOT;
const Container *Container::dragContainer = 0;

size_t Container::useSlot = NO_SLOT;
const Container *Container::useContainer = 0;

Container::Container(size_t rows, size_t cols, Item::vect_t &linked, size_t serial, int x, int y):
Element(Rect(x, y,
                 cols * (Client::ICON_SIZE + GAP + 2) + GAP,
                 rows * (Client::ICON_SIZE + GAP + 2) + GAP + 1)),
_rows(rows),
_cols(cols),
_linked(linked),
_serial(serial),
_leftMouseDownSlot(NO_SLOT),
_rightMouseDownSlot(NO_SLOT){
    for (size_t i = 0; i != Client::INVENTORY_SIZE; ++i) {
        const int
            x = i % cols,
            y = i / cols;
        const Rect slotRect = Rect(x * (Client::ICON_SIZE + GAP + 2) + GAP,
                                   y * (Client::ICON_SIZE + GAP + 2) + GAP + 1,
                                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
        addChild(new ShadowBox(slotRect, true));
    }
    setLeftMouseDownFunction(leftMouseDown);
    setLeftMouseUpFunction(leftMouseUp);
    setRightMouseDownFunction(rightMouseDown);
    setRightMouseUpFunction(rightMouseUp);
}

void Container::refresh(){
    renderer.setDrawColor(Color::BLACK);
    for (size_t i = 0; i != Client::INVENTORY_SIZE; ++i) {
        const int
            x = i % _cols,
            y = i / _cols;
        const Rect slotRect = Rect(x * (Client::ICON_SIZE + GAP + 2) + GAP,
                                            y * (Client::ICON_SIZE + GAP + 2) + GAP + 1,
                                            Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
        static const Rect SLOT_BACKGROUND_OFFSET = Rect(1, 1, -2, -2);
        renderer.fillRect(slotRect + SLOT_BACKGROUND_OFFSET);
        if (dragSlot == i) // Don't draw an item being moved by the mouse.
            continue;
        const std::pair<const Item *, size_t> &slot = _linked[i];
        if (slot.first){
            slot.first->icon().draw(slotRect.x + 1, slotRect.y + 1);
            if (slot.second > 1){
                Texture label(font(), makeArgs(slot.second), FONT_COLOR);
                label.draw(slotRect.x + slotRect.w - label.width() - 1,
                           slotRect.y + slotRect.h - label.height() + 1 + textOffset);
            }
        }
    }
}

size_t Container::getSlot(const Point &mousePos) const{
    size_t
        x = static_cast<size_t>((mousePos.x - GAP) / (Client::ICON_SIZE + GAP + 2)),
        y = static_cast<size_t>((mousePos.y - GAP - 1) / (Client::ICON_SIZE + GAP + 2));
    size_t slot = y * _cols + x;
    if (slot >= Client::INVENTORY_SIZE)
        return NO_SLOT;
    return slot;
}

void Container::leftMouseDown(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    container._leftMouseDownSlot = slot;
}

void Container::leftMouseUp(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    if (slot != NO_SLOT) { // Clicked a valid slot
        if (dragSlot != slot && dragSlot != NO_SLOT) { // Different slot: finish dragging
            Client::_instance->sendMessage(CL_SWAP_ITEMS, makeArgs(dragContainer->_serial, dragSlot,
                                                                   container._serial, slot));
            dragSlot = NO_SLOT;
            dragContainer = 0;
        } else if (slot == dragSlot && &container == dragContainer) {
            dragSlot = NO_SLOT;
            dragContainer = 0;
            container.markChanged();
        } else if (container._leftMouseDownSlot == slot && // Same slot that mouse went down on
                    container._linked[slot].first) { // and slot isn't empty: start dragging
            dragSlot = slot;
            dragContainer = &container;
            container.markChanged();
        }
    }
    container._leftMouseDownSlot = NO_SLOT;
}

void Container::rightMouseDown(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    container._rightMouseDownSlot = slot;
}

void Container::rightMouseUp(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    if (dragSlot != NO_SLOT) { // Cancel dragging
        dragSlot = NO_SLOT;
        dragContainer = 0;
        container.markChanged();
    }
    size_t slot = container.getSlot(mousePos);
    if (useSlot != NO_SLOT) { // Right-clicked instead of used: cancel use
        useSlot = NO_SLOT;
        useContainer = 0;
    } else if (slot != NO_SLOT) { // Right-clicked a slot
        const Item *item = container._linked[slot].first;
        if (item && // Slot is not empty
            item->constructsObject()) { // Can construct item
            useSlot = slot;
            useContainer = &container;
        }
    }
    container._rightMouseDownSlot = NO_SLOT;
}

const Item *Container::getDragItem() {
    if (dragSlot == NO_SLOT || !dragContainer)
        return 0;
    else
        return dragContainer->_linked[dragSlot].first;
}

const Item *Container::getUseItem() {
    if (useSlot == NO_SLOT || !useContainer)
        return 0;
    else
        return useContainer->_linked[useSlot].first;
}

void Container::dropItem() {
    if (dragSlot != NO_SLOT && dragContainer) {
        Client::_instance->sendMessage(CL_DROP, makeArgs(dragContainer->_serial, dragSlot));
        dragSlot = NO_SLOT;
        dragContainer = 0;
    }
}

void Container::clearUseItem() {
    useSlot = NO_SLOT;
    useContainer = 0;
}
