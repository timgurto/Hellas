#ifndef CONTAINER_GRID_H
#define CONTAINER_GRID_H

#include "../../Serial.h"
#include "../ClientItem.h"
#include "Element.h"

class Client;

// A grid that allows access to a collection of items
class ContainerGrid : public Element {
  static const px_t DEFAULT_GAP;

  size_t _rows, _cols;
  ClientItem::vect_t &_linked;
  Serial _serial;         // The serial of the owning object.
  px_t _gap;              // Spacing between grid squares.
  bool _solidBackground;  // Whether to draw a dark square behind each slot.

  size_t _mouseOverSlot{NO_SLOT},   // The slot that the mouse is currently over
      _leftMouseDownSlot{NO_SLOT},  // The slot that the left mouse button went
                                    // down on, if any.
      _rightMouseDownSlot{NO_SLOT};  // The slot that the left mouse button went
                                     // down on, if any.

  static const size_t NO_SLOT;

  virtual void refresh() override;
  void refreshTooltip();

  static void leftMouseDown(Element &e, const ScreenPoint &mousePos);
  static void leftMouseUp(Element &e, const ScreenPoint &mousePos);
  static void rightMouseDown(Element &e, const ScreenPoint &mousePos);
  static void rightMouseUp(Element &e, const ScreenPoint &mousePos);
  static void mouseMove(Element &e, const ScreenPoint &mousePos);

  size_t getSlot(const ScreenPoint &mousePos) const;
  void clearMouseOver();

 public:
  ContainerGrid(Client &client, size_t rows, size_t cols,
                ClientItem::vect_t &linked, Serial serial = Serial::Inventory(),
                px_t x = 0, px_t y = 0, px_t gap = DEFAULT_GAP,
                bool solidBackground = true);
  ~ContainerGrid();

  static void dropItem(Client &client);   // Drop the item being dragged.
  static void scrapItem(Client &client);  // Scrap the item being dragged.

  friend Client;

  class GridInUse {
   public:
    GridInUse(const ContainerGrid &grid, size_t slot);
    GridInUse(){};
    bool validGrid() const;
    bool validSlot() const;
    size_t slot() const;
    bool slotMatches(size_t slot) const;
    bool matches(const ContainerGrid &grid, size_t slot) const;
    const ClientItem *item() const;
    const Serial object() const;
    void clear();
    void markGridAsChanged();
    bool isItemSoulbound() const;

   private:
    size_t _slot{ContainerGrid::NO_SLOT};
    const ContainerGrid *_grid{nullptr};
  };
};

#endif
