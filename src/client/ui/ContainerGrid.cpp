#include "ContainerGrid.h"

#include "../../server/User.h"
#include "../Client.h"
#include "../Renderer.h"
#include "../Texture.h"
#include "ColorBlock.h"

extern Renderer renderer;

const px_t ContainerGrid::DEFAULT_GAP = 0;

// TODO: find better alternative.
const size_t ContainerGrid::NO_SLOT = 999;

size_t ContainerGrid::dragSlot = NO_SLOT;
const ContainerGrid *ContainerGrid::dragGrid = nullptr;

size_t ContainerGrid::useSlot = NO_SLOT;
const ContainerGrid *ContainerGrid::useGrid = nullptr;

Texture ContainerGrid::_highlight;
Texture ContainerGrid::_highlightGood;
Texture ContainerGrid::_highlightBad;
Texture ContainerGrid::_damaged;
Texture ContainerGrid::_broken;

ContainerGrid::ContainerGrid(size_t rows, size_t cols,
                             ClientItem::vect_t &linked, Serial serial, px_t x,
                             px_t y, px_t gap, bool solidBackground)
    : Element(
          {x, y, static_cast<px_t>(cols) * (Client::ICON_SIZE + gap + 2) + gap,
           static_cast<px_t>(rows) * (Client::ICON_SIZE + gap + 2) + gap + 1}),
      _rows(rows),
      _cols(cols),
      _linked(linked),
      _serial(serial),
      _mouseOverSlot(NO_SLOT),
      _leftMouseDownSlot(NO_SLOT),
      _rightMouseDownSlot(NO_SLOT),
      _gap(gap),
      _solidBackground(solidBackground) {
  if (!_highlight) {
    _highlight = {"Images/Items/highlight.png"s, Color::MAGENTA};
    _highlightGood = {"Images/Items/highlightGood.png"s, Color::MAGENTA};
    _highlightBad = {"Images/Items/highlightBad.png"s, Color::MAGENTA};

    _damaged = {Client::ICON_SIZE, Client::ICON_SIZE};
    renderer.pushRenderTarget(_damaged);
    renderer.setDrawColor(Color::DURABILITY_LOW);
    renderer.fill();
    renderer.popRenderTarget();
    _damaged.setAlpha(0x7f);
    _damaged.setBlend();

    _broken = {Client::ICON_SIZE, Client::ICON_SIZE};
    renderer.pushRenderTarget(_broken);
    renderer.setDrawColor(Color::DURABILITY_BROKEN);
    renderer.fill();
    renderer.popRenderTarget();
    _broken.setAlpha(0x7f);
    _broken.setBlend();
  }

  for (size_t i = 0; i != _linked.size(); ++i) {
    const px_t x = i % cols, y = i / cols;
    const auto slotRect =
        ScreenRect{x * (Client::ICON_SIZE + _gap + 2),
                   y * (Client::ICON_SIZE + _gap + 2) + 1,
                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2};
    addChild(new ShadowBox(slotRect, true));
  }
  setLeftMouseDownFunction(leftMouseDown);
  setLeftMouseUpFunction(leftMouseUp);
  setRightMouseDownFunction(rightMouseDown);
  setRightMouseUpFunction(rightMouseUp);
  setMouseMoveFunction(mouseMove);
}

void ContainerGrid::cleanup() {
  _highlight = {};
  _highlightGood = {};
  _highlightBad = {};
  _damaged = {};
  _broken = {};
}

void ContainerGrid::refresh() {
  renderer.setDrawColor(Color::BLACK);
  for (size_t i = 0; i != _linked.size(); ++i) {
    const px_t x = i % _cols, y = i / _cols;
    const auto slotRect =
        ScreenRect{x * (Client::ICON_SIZE + _gap + 2),
                   y * (Client::ICON_SIZE + _gap + 2) + 1,
                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2};
    if (_solidBackground) {
      static const auto SLOT_BACKGROUND_OFFSET = ScreenRect{1, 1, -2, -2};
      renderer.fillRect(slotRect + SLOT_BACKGROUND_OFFSET);
    }
    // Don't draw an item being moved by the mouse.
    if (dragSlot != i || dragGrid != this) {
      const auto &slot = _linked[i];
      if (slot.first.type() != nullptr) {
        slot.first.type()->icon().draw(slotRect.x + 1, slotRect.y + 1);

        if (slot.first.health() == 0)
          _broken.draw(slotRect.x + 1, slotRect.y + 1);
        else if (slot.first.health() <= 20)
          _damaged.draw(slotRect.x + 1, slotRect.y + 1);

        if (slot.second > 1) {
          Texture label(font(), makeArgs(slot.second), FONT_COLOR),
              labelOutline(font(), toString(slot.second), Color::UI_OUTLINE);
          px_t x = slotRect.x + slotRect.w - label.width() - 1,
               y = slotRect.y + slotRect.h - label.height() + textOffset;
          labelOutline.draw(x - 1, y);
          labelOutline.draw(x + 1, y);
          labelOutline.draw(x, y - 1);
          labelOutline.draw(x, y + 1);
          label.draw(x, y);
        }
      }
    }

    // Highlight moused-over slot
    if (_mouseOverSlot == i) {
      _highlight.draw(slotRect.x + 1, slotRect.y + 1);

      // Indicate matching gear slot if an item is being dragged
    } else if (_serial.isGear() && dragGrid != nullptr) {
      size_t itemSlot = dragGrid->_linked[dragSlot].first.type()->gearSlot();
      (i == itemSlot ? _highlightGood : _highlightBad)
          .draw(slotRect.x + 1, slotRect.y + 1);
    }
  }

  refreshTooltip();
}

void ContainerGrid::refreshTooltip() {
  if (_mouseOverSlot != NO_SLOT) {
    const auto &item = _linked[_mouseOverSlot].first;
    if (!item.type())
      clearTooltip();
    else
      setTooltip(item.tooltip());
  }
}

size_t ContainerGrid::getSlot(const ScreenPoint &mousePos) const {
  if (!collision(mousePos, ScreenRect{0, 0, width(), height()})) return NO_SLOT;
  size_t x = static_cast<size_t>((mousePos.x) / (Client::ICON_SIZE + _gap + 2)),
         y = static_cast<size_t>((mousePos.y - 1) /
                                 (Client::ICON_SIZE + _gap + 2));
  // Check inside gaps
  if (mousePos.x - static_cast<px_t>(x) * (Client::ICON_SIZE + _gap + 2) >
      Client::ICON_SIZE + 2)
    return NO_SLOT;
  if (mousePos.y - static_cast<px_t>(y) * (Client::ICON_SIZE + _gap + 2) >
      Client::ICON_SIZE + 2)
    return NO_SLOT;
  size_t slot = y * _cols + x;
  if (slot >= _linked.size()) return NO_SLOT;
  return slot;
}

void ContainerGrid::leftMouseDown(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);

  const auto &client = Client::instance();
  if (client.isAltPressed())
    client.sendMessage({CL_REPAIR_ITEM, makeArgs(grid._serial, slot)});
  else
    grid._leftMouseDownSlot = slot;
}

void ContainerGrid::leftMouseUp(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);
  if (slot != NO_SLOT) {  // Clicked a valid slot
    size_t mouseDownSlot = grid._leftMouseDownSlot;
    grid._leftMouseDownSlot = NO_SLOT;

    // Enforce gear slots
    if (dragGrid != nullptr) {
      if (dragGrid->_serial.isGear()) {  // From gear slot
        const ClientItem *item = grid._linked[slot].first.type();
        if (item != nullptr && item->gearSlot() != dragSlot) return;
      } else if (grid._serial.isGear()) {  // To gear slot
        const ClientItem *item = dragGrid->_linked[dragSlot].first.type();
        if (item != nullptr && item->gearSlot() != slot) return;
      }
    }

    // Different grid/slot: finish dragging.
    if ((dragGrid != &grid || dragSlot != slot) && dragSlot != NO_SLOT) {
      Client::_instance->sendMessage(
          {CL_SWAP_ITEMS,
           makeArgs(dragGrid->_serial, dragSlot, grid._serial, slot)});
      const ClientItem *item = dragGrid->_linked[dragSlot].first.type();
      if (item != nullptr) item->playSoundOnce("drop");

      dragSlot = NO_SLOT;
      dragGrid = nullptr;
      Client::_instance->onChangeDragItem();

      // Dragging to same grid/slot; do nothing.
    } else if (slot == dragSlot && &grid == dragGrid) {
      dragSlot = NO_SLOT;
      dragGrid = nullptr;
      Client::_instance->onChangeDragItem();
      grid.markChanged();

      // Same grid and slot that mouse went down on and slot isn't empty: start
      // dragging.
    } else if (mouseDownSlot == slot && grid._linked[slot].first.type()) {
      dragSlot = slot;
      dragGrid = &grid;
      Client::_instance->onChangeDragItem();
      grid.markChanged();
    }
  }
}

void ContainerGrid::rightMouseDown(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);
  grid._rightMouseDownSlot = slot;
}

void ContainerGrid::rightMouseUp(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);
  if (dragSlot != NO_SLOT) {  // Cancel dragging
    dragSlot = NO_SLOT;
    dragGrid = nullptr;
    Client::_instance->onChangeDragItem();
    grid.markChanged();
  }
  if (useSlot != NO_SLOT) {  // Right-clicked instead of used: cancel use
    useSlot = NO_SLOT;
    useGrid = nullptr;
  } else if (slot != NO_SLOT) {  // Right-clicked a slot
    const ClientItem *item = grid._linked[slot].first.type();
    if (item != nullptr) {  // Slot is not empty
      if (grid._serial.isInventory()) {
        if (item->canUse()) {
          useSlot = slot;
          useGrid = &grid;
        } else if (item->gearSlot() < Client::GEAR_SLOTS) {
          Client::_instance->sendMessage(
              {CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), slot,
                                       Serial::Gear(), item->gearSlot())});
          item->playSoundOnce("drop");
        }
      } else {  // An object: take item
        Client::_instance->sendMessage(
            {CL_TAKE_ITEM, makeArgs(grid._serial, slot)});
        item->playSoundOnce("drop");
      }
    }
  }
  grid._rightMouseDownSlot = NO_SLOT;
}

void ContainerGrid::mouseMove(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);
  if (slot != grid._mouseOverSlot) {
    grid._mouseOverSlot = slot;
    grid.markChanged();
  }

  grid.refreshTooltip();
}

const ClientItem *ContainerGrid::getDragItem() {
  if (dragSlot == NO_SLOT || !dragGrid)
    return nullptr;
  else
    return dragGrid->_linked[dragSlot].first.type();
}

const ClientItem *ContainerGrid::getUseItem() {
  if (useSlot == NO_SLOT || !useGrid)
    return nullptr;
  else
    return useGrid->_linked[useSlot].first.type();
}

void ContainerGrid::dropItem() {
  if (dragSlot != NO_SLOT && dragGrid != nullptr) {
    const ClientItem *item = dragGrid->_linked[dragSlot].first.type();
    Client::_instance->dropItemOnConfirmation(dragGrid->_serial, dragSlot,
                                              item);
    dragSlot = NO_SLOT;
    dragGrid->markChanged();
    dragGrid = nullptr;
    Client::_instance->onChangeDragItem();
  }
}

void ContainerGrid::clearUseItem() {
  useSlot = NO_SLOT;
  useGrid = nullptr;
}
