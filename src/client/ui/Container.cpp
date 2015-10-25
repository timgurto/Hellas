// (C) 2015 Tim Gurto

#include "ColorBlock.h"
#include "Container.h"
#include "../Client.h"
#include "../Renderer.h"
#include "../../server/User.h"

extern Renderer renderer;

const int Container::GAP = 0;

Container::Container(size_t rows, size_t cols, Item::vect_t &linked, int x, int y):
Element(Rect(x, y,
                 cols * (Client::ICON_SIZE + GAP + 2) + GAP,
                 rows * (Client::ICON_SIZE + GAP + 2) + GAP + 1)),
_rows(rows),
_cols(cols),
_linked(linked),
_leftMouseDownSlot(Client::INVENTORY_SIZE),
_leftClickSlot(Client::INVENTORY_SIZE){
    for (size_t i = 0; i != Client::INVENTORY_SIZE; ++i) {
        const int
            x = i % cols,
            y = i / cols;
        const Rect slotRect = Rect(x * (Client::ICON_SIZE + GAP + 2) + GAP,
                                   y * (Client::ICON_SIZE + GAP + 2) + GAP + 1,
                                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
        addChild(new ShadowBox(slotRect, true));
    }
    setMouseDownFunction(mouseDown);
    setMouseUpFunction(mouseUp);
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
        if (_leftClickSlot == i) // Don't draw an item being moved by the mouse.
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
    return min(slot, Client::INVENTORY_SIZE);
}

void Container::mouseDown(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    container._leftMouseDownSlot = container.getSlot(mousePos);
    if (!container._linked[container._leftMouseDownSlot].first)
        container._leftMouseDownSlot = Client::INVENTORY_SIZE;
}

void Container::mouseUp(Element &e, const Point &mousePos){
    e.markChanged();
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    if (container._leftClickSlot == slot) { // User dropped item in same location.
        container._leftClickSlot = Client::INVENTORY_SIZE;
        return;
    }
    if (container._leftMouseDownSlot == slot) {
        container._leftClickSlot = slot;
        container._leftMouseDownSlot = Client::INVENTORY_SIZE;
    } else {
        container._leftMouseDownSlot = Client::INVENTORY_SIZE;
        container._leftClickSlot = Client::INVENTORY_SIZE;
    }
}

void Container::clearMouseDown(){
    _leftClickSlot = Client::INVENTORY_SIZE;
}
