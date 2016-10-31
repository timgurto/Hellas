#include "ColorBlock.h"
#include "Container.h"
#include "../Client.h"
#include "../Renderer.h"
#include "../Texture.h"
#include "../../server/User.h"

extern Renderer renderer;

const px_t Container::DEFAULT_GAP = 0;

// TODO: find better alternative.
const size_t Container::NO_SLOT = 999;

size_t Container::dragSlot = NO_SLOT;
const Container *Container::dragContainer = nullptr;

size_t Container::useSlot = NO_SLOT;
const Container *Container::useContainer = nullptr;

Texture Container::_highlight;
Texture Container::_highlightGood;
Texture Container::_highlightBad;

Container::Container(size_t rows, size_t cols, ClientItem::vect_t &linked, size_t serial,
                     px_t x, px_t y, px_t gap, bool solidBackground):
Element(Rect(x, y,
             cols * (Client::ICON_SIZE + gap + 2) + gap,
             rows * (Client::ICON_SIZE + gap + 2) + gap + 1)),
_rows(rows),
_cols(cols),
_linked(linked),
_serial(serial),
_mouseOverSlot(NO_SLOT),
_leftMouseDownSlot(NO_SLOT),
_rightMouseDownSlot(NO_SLOT),
_gap(gap),
_solidBackground(solidBackground){
    if (!_highlight){
        _highlight = Texture(std::string("Images/Items/highlight.png"), Color::MAGENTA);
        _highlightGood = Texture(std::string("Images/Items/highlightGood.png"), Color::MAGENTA);
        _highlightBad = Texture(std::string("Images/Items/highlightBad.png"), Color::MAGENTA);
    }

    for (size_t i = 0; i != _linked.size(); ++i) {
        const px_t
            x = i % cols,
            y = i / cols;
        const Rect slotRect = Rect(x * (Client::ICON_SIZE + _gap + 2),
                                   y * (Client::ICON_SIZE + _gap + 2) + 1,
                                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
        addChild(new ShadowBox(slotRect, true));
    }
    setLeftMouseDownFunction(leftMouseDown);
    setLeftMouseUpFunction(leftMouseUp);
    setRightMouseDownFunction(rightMouseDown);
    setRightMouseUpFunction(rightMouseUp);
    setMouseMoveFunction(mouseMove);
}

void Container::refresh(){
    renderer.setDrawColor(Color::MMO_OUTLINE);
    for (size_t i = 0; i != _linked.size(); ++i) {
        const px_t
            x = i % _cols,
            y = i / _cols;
        const Rect slotRect = Rect(x * (Client::ICON_SIZE + _gap + 2),
                                   y * (Client::ICON_SIZE + _gap + 2) + 1,
                                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
        if (_solidBackground){
            static const Rect SLOT_BACKGROUND_OFFSET = Rect(1, 1, -2, -2);
            renderer.fillRect(slotRect + SLOT_BACKGROUND_OFFSET);
        }
        if (dragSlot != i || dragContainer != this) { // Don't draw an item being moved by the mouse.
            const std::pair<const ClientItem *, size_t> &slot = _linked[i];
            if (slot.first != nullptr){
                slot.first->icon().draw(slotRect.x + 1, slotRect.y + 1);
                if (slot.second > 1){
                    Texture
                        label(font(), makeArgs(slot.second), FONT_COLOR),
                        labelOutline(font(), makeArgs(slot.second), Color::MMO_OUTLINE);
                    px_t
                        x = slotRect.x + slotRect.w - label.width() - 1,
                        y = slotRect.y + slotRect.h - label.height() + 1 + textOffset;
                    labelOutline.draw(x-1, y);
                    labelOutline.draw(x+1, y);
                    labelOutline.draw(x, y-1);
                    labelOutline.draw(x, y+1);
                    label.draw(x, y);
                }
            }
        }

        // Highlight moused-over slot
        if (_mouseOverSlot == i) {
            _highlight.draw(slotRect.x + 1, slotRect.y + 1);

        // Indicate matching gear slot if an item is being dragged
        } else if (_serial == Client::GEAR && dragContainer != nullptr){
            size_t itemSlot = dragContainer->_linked[dragSlot].first->gearSlot();
            (i == itemSlot ? _highlightGood : _highlightBad).draw(slotRect.x + 1, slotRect.y + 1);
        }
    }
}

size_t Container::getSlot(const Point &mousePos) const{
    if (!collision(mousePos, Rect(0, 0, width(), height())))
        return NO_SLOT;
    size_t
        x = static_cast<size_t>((mousePos.x) / (Client::ICON_SIZE + _gap + 2)),
        y = static_cast<size_t>((mousePos.y - 1) / (Client::ICON_SIZE + _gap + 2));
    // Check inside gaps
    if (mousePos.x - x * (Client::ICON_SIZE + _gap + 2) > Client::ICON_SIZE + 2)
        return NO_SLOT;
    if (mousePos.y - y * (Client::ICON_SIZE + _gap + 2) > Client::ICON_SIZE + 2)
        return NO_SLOT;
    size_t slot = y * _cols + x;
    if (slot >= _linked.size())
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
        size_t mouseDownSlot = container._leftMouseDownSlot;
        container._leftMouseDownSlot = NO_SLOT;

        // Enforce gear slots
        if (dragContainer != nullptr){
            if (dragContainer->_serial == Client::GEAR){ // From gear slot
                const ClientItem *item = container._linked[slot].first;
                if (item != nullptr && item->gearSlot() != dragSlot)
                    return;
            } else if (container._serial == Client::GEAR){ // To gear slot
                const ClientItem *item = dragContainer->_linked[dragSlot].first;
                if (item != nullptr && item->gearSlot() != slot)
                    return;
            }
        }

        // Different container/slot: finish dragging.
        if ((dragContainer != &container || dragSlot != slot) && dragSlot != NO_SLOT) { 
            Client::_instance->sendMessage(CL_SWAP_ITEMS, makeArgs(dragContainer->_serial, dragSlot,
                                                                   container._serial, slot));
            dragSlot = NO_SLOT;
            dragContainer = nullptr;
            Client::_instance->onChangeDragItem();

        // Dragging to same container/slot; do nothing.
        } else if (slot == dragSlot && &container == dragContainer) {
            dragSlot = NO_SLOT;
            dragContainer = nullptr;
            Client::_instance->onChangeDragItem();
            container.markChanged();

        // Same container and slot that mouse went down on and slot isn't empty: start dragging.
        } else if (mouseDownSlot == slot &&  container._linked[slot].first) {
            dragSlot = slot;
            dragContainer = &container;
            Client::_instance->onChangeDragItem();
            container.markChanged();
        }
    }
}

void Container::rightMouseDown(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    container._rightMouseDownSlot = slot;
}

void Container::rightMouseUp(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    if (dragSlot != NO_SLOT) { // Cancel dragging
        dragSlot = NO_SLOT;
        dragContainer = nullptr;
        Client::_instance->onChangeDragItem();
        container.markChanged();
    }
    if (useSlot != NO_SLOT) { // Right-clicked instead of used: cancel use
        useSlot = NO_SLOT;
        useContainer = nullptr;
    } else if (slot != NO_SLOT) { // Right-clicked a slot
        const ClientItem *item = container._linked[slot].first;
        if (item != nullptr){ // Slot is not empty
            if (container._serial == Client::INVENTORY) {
                if (item->constructsObject() != nullptr) { // Can construct item
                    useSlot = slot;
                    useContainer = &container;
                }
            } else { // An object: take item
                Client::_instance->sendMessage(CL_TAKE_ITEM, makeArgs(container._serial, slot));
            }
        }
    }
    container._rightMouseDownSlot = NO_SLOT;
}

void Container::mouseMove(Element &e, const Point &mousePos){
    Container &container = dynamic_cast<Container &>(e);
    size_t slot = container.getSlot(mousePos);
    if (slot != container._mouseOverSlot) {
        container._mouseOverSlot = slot;
        container.markChanged();
    }
}

const ClientItem *Container::getDragItem() {
    if (dragSlot == NO_SLOT || !dragContainer)
        return nullptr;
    else
        return dragContainer->_linked[dragSlot].first;
}

const ClientItem *Container::getUseItem() {
    if (useSlot == NO_SLOT || !useContainer)
        return nullptr;
    else
        return useContainer->_linked[useSlot].first;
}

void Container::dropItem() {
    if (dragSlot != NO_SLOT && dragContainer) {
        Client::_instance->dropItemOnConfirmation(dragContainer->_serial, dragSlot,
                                                  dragContainer->_linked[dragSlot].first);
        dragSlot = NO_SLOT;
        dragContainer->markChanged();
        dragContainer = nullptr;
        Client::_instance->onChangeDragItem();
    }
}

void Container::clearUseItem() {
    useSlot = NO_SLOT;
    useContainer = nullptr;
}
