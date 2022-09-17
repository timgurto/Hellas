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

ContainerGrid::ContainerGrid(Client &client, size_t rows, size_t cols,
                             ClientItem::vect_t &linked, Serial serial, px_t x,
                             px_t y, px_t gap, bool solidBackground)
    : Element(
          {x, y, static_cast<px_t>(cols) * (Client::ICON_SIZE + gap + 2) + gap,
           static_cast<px_t>(rows) * (Client::ICON_SIZE + gap + 2) + gap + 1}),
      _rows(rows),
      _cols(cols),
      _linked(linked),
      _serial(serial),
      _gap(gap),
      _solidBackground(solidBackground) {
  setClient(client);
  client.registerContainerGrid(this);

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

ContainerGrid::~ContainerGrid() { _client->deregisterContainerGrid(this); }

void ContainerGrid::refresh() {
  static const auto SLOT_BACKGROUND_OFFSET = ScreenRect{1, 1, -2, -2};

  renderer.setDrawColor(Color::BLACK);
  for (size_t i = 0; i != _linked.size(); ++i) {
    const px_t x = i % _cols, y = i / _cols;
    const auto slotRect =
        ScreenRect{x * (Client::ICON_SIZE + _gap + 2),
                   y * (Client::ICON_SIZE + _gap + 2) + 1,
                   Client::ICON_SIZE + 2, Client::ICON_SIZE + 2};
    if (_solidBackground) {
      renderer.setDrawColor(Color::BLACK);
      renderer.fillRect(slotRect + SLOT_BACKGROUND_OFFSET);
    }
    // Don't draw an item being moved by the mouse.
    const auto &draggingFrom = client()->containerGridBeingDraggedFrom;
    if (!draggingFrom.matches(*this, i)) {
      const auto &slot = _linked[i];
      const auto *itemType = slot.first.type();
      if (itemType) {
        // Solid background (even if set to false)
        if (slot.first.shouldDrawAsBroken()) {
          renderer.setDrawColor(Color::DURABILITY_BROKEN);
          renderer.fillRect(slotRect + SLOT_BACKGROUND_OFFSET);
        } else if (slot.first.shouldDrawAsDamaged()) {
          renderer.setDrawColor(Color::DURABILITY_LOW);
          renderer.fillRect(slotRect + SLOT_BACKGROUND_OFFSET);
        }

        // Quality border
        const auto qualityColour = itemType->nameColor();
        if (qualityColour != Color::ITEM_QUALITY_COMMON) {
          _client->images.itemQualityMask.setColourMod(qualityColour);
          _client->images.itemQualityMask.draw(slotRect.x + 1, slotRect.y + 1);
        }

        slot.first.type()->icon().draw(slotRect.x + 1, slotRect.y + 1);

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
      Client::images.itemHighlightMouseOver.draw(slotRect.x + 1,
                                                 slotRect.y + 1);

      // Indicate matching gear slot if an item is being dragged
    } else if (_serial.isGear() && draggingFrom.validGrid()) {
      auto itemSlot = draggingFrom.item()->type()->gearSlot();
      (i == itemSlot ? Client::images.itemHighlightMatch
                     : Client::images.itemHighlightNoMatch)
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

void ContainerGrid::clearMouseOver() {
  if (_mouseOverSlot == NO_SLOT) return;
  _mouseOverSlot = NO_SLOT;
  markChanged();
}

void ContainerGrid::leftMouseDown(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);

  if (grid._client->isAltPressed())
    grid._client->sendMessage({CL_REPAIR_ITEM, makeArgs(grid._serial, slot)});
  else
    grid._leftMouseDownSlot = slot;
}

void ContainerGrid::leftMouseUp(Element &e, const ScreenPoint &mousePos) {
  ContainerGrid &grid = dynamic_cast<ContainerGrid &>(e);
  size_t slot = grid.getSlot(mousePos);
  if (slot == NO_SLOT) return;  // Clicked an invalid slot

  size_t mouseDownSlot = grid._leftMouseDownSlot;
  grid._leftMouseDownSlot = NO_SLOT;

  // Enforce gear slots
  auto &draggingFrom = grid.client()->containerGridBeingDraggedFrom;
  if (draggingFrom.validGrid()) {
    if (draggingFrom.object().isGear()) {  // From gear slot
      const auto *item = grid._linked[slot].first.type();
      if (item && !draggingFrom.slotMatches(item->gearSlot())) return;
    } else if (grid._serial.isGear()) {  // To gear slot
      const auto *item = draggingFrom.item();
      if (item && item->type()->gearSlot() != slot) return;
    }
  }

  // Different grid/slot: finish dragging.
  if (!draggingFrom.matches(grid, slot) && draggingFrom.validSlot()) {
    grid._client->sendMessage(
        {CL_SWAP_ITEMS, makeArgs(draggingFrom.object(), draggingFrom.slot(),
                                 grid._serial, slot)});
    const auto *item = draggingFrom.item();
    if (item) item->type()->playSoundOnce(*grid.client(), "drop");

    draggingFrom.clear();
    grid._client->onChangeDragItem();

    // Dragging to same grid/slot; do nothing.
  } else if (draggingFrom.matches(grid, slot)) {
    draggingFrom.clear();
    grid._client->onChangeDragItem();
    grid.markChanged();

    // Same grid and slot that mouse went down on and slot isn't empty: start
    // dragging.
  } else if (mouseDownSlot == slot && grid._linked[slot].first.type()) {
    draggingFrom = {grid, slot};
    grid._client->onChangeDragItem();
    grid.markChanged();
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
  auto &draggingFrom = grid.client()->containerGridBeingDraggedFrom;
  auto &usingFrom = grid.client()->containerGridInUse;
  if (!draggingFrom.validSlot()) {  // Cancel dragging
    draggingFrom.clear();
    grid._client->onChangeDragItem();
    grid.markChanged();
  }
  if (usingFrom.validSlot()) {  // Right-clicked instead of used: cancel use
    usingFrom.clear();
  } else if (slot != NO_SLOT) {  // Right-clicked a slot
    const ClientItem *item = grid._linked[slot].first.type();
    if (item != nullptr) {  // Slot is not empty
      if (grid._serial.isInventory()) {
        if (item->canUse()) {
          usingFrom = {grid, slot};
        } else if (item->gearSlot() != Item::NOT_GEAR) {
          grid._client->sendMessage(
              {CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), slot,
                                       Serial::Gear(), item->gearSlot())});
          item->playSoundOnce(*grid.client(), "drop");
        }
      } else {  // An object: take item
        grid._client->sendMessage({CL_TAKE_ITEM, makeArgs(grid._serial, slot)});
        item->playSoundOnce(*grid.client(), "drop");
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

void ContainerGrid::dropItem(Client &client) {
  auto &draggingFrom = client.containerGridBeingDraggedFrom;
  if (!draggingFrom.validGrid() || !draggingFrom.validSlot()) return;

  if (draggingFrom.isItemSoulbound())
    client.dropItemOnConfirmation(draggingFrom);
  else
    client.sendMessage(
        {CL_DROP, makeArgs(draggingFrom.object(), draggingFrom.slot())});
  draggingFrom.markGridAsChanged();
  draggingFrom.clear();
  client.onChangeDragItem();
}

void ContainerGrid::scrapItem(Client &client) {
  auto &draggingFrom = client.containerGridBeingDraggedFrom;
  if (!draggingFrom.validGrid() || !draggingFrom.validSlot()) return;

  if (draggingFrom.item()->type()->shouldWarnBeforeScrapping())
    client.scrapItemOnConfirmation(draggingFrom, draggingFrom.item()->name());
  else
    client.sendMessage(
        {CL_SCRAP_ITEM, makeArgs(draggingFrom.object(), draggingFrom.slot())});
  draggingFrom.markGridAsChanged();
  draggingFrom.clear();
  client.onChangeDragItem();
}

ContainerGrid::GridInUse::GridInUse(const ContainerGrid &grid, size_t slot)
    : _grid(&grid), _slot(slot) {}

bool ContainerGrid::GridInUse::validGrid() const { return _grid != nullptr; }

bool ContainerGrid::GridInUse::validSlot() const { return _slot != NO_SLOT; }

size_t ContainerGrid::GridInUse::slot() const { return _slot; }

bool ContainerGrid::GridInUse::slotMatches(size_t slot) const {
  return _slot == slot;
}

bool ContainerGrid::GridInUse::matches(const ContainerGrid &grid,
                                       size_t slot) const {
  return _grid == &grid && _slot == slot;
}

const ClientItem::Instance *ContainerGrid::GridInUse::item() const {
  if (!validGrid() || !validSlot()) return nullptr;
  return &_grid->_linked[_slot].first;
}

const Serial ContainerGrid::GridInUse::object() const { return _grid->_serial; }

void ContainerGrid::GridInUse::clear() {
  _slot = NO_SLOT;
  _grid = nullptr;
}

void ContainerGrid::GridInUse::markGridAsChanged() { _grid->markChanged(); }

bool ContainerGrid::GridInUse::isItemSoulbound() const {
  if (!validGrid() || !validSlot()) return false;
  return _grid->_linked[_slot].first.isSoulbound();
}
