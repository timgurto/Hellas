#ifndef CONTAINER_GRID_H
#define CONTAINER_GRID_H

#include "../../Serial.h"
#include "../Client.h"
#include "../ClientItem.h"
#include "Element.h"

// A grid that allows access to a collection of items
class ContainerGrid : public Element {
  static const px_t DEFAULT_GAP;

  size_t _rows, _cols;
  ClientItem::vect_t &_linked;
  Serial _serial;         // The serial of the owning object.
  px_t _gap;              // Spacing between grid squares.
  bool _solidBackground;  // Whether to draw a dark square behind each slot.

  size_t _mouseOverSlot,    // The slot that the mouse is currently over
      _leftMouseDownSlot,   // The slot that the left mouse button went down on,
                            // if any.
      _rightMouseDownSlot;  // The slot that the left mouse button went down on,
                            // if any.

  static const size_t NO_SLOT;

  static Texture _highlight,  // Emphasizes any slot the mouse is over.
      _highlightGood,         // Used to indicate matching gear slots.
      _highlightBad;
  static Texture _damaged, _broken;  // Drawn over icons to indicate durability

  static size_t dragSlot;  // The slot currently being dragged from.
  static const ContainerGrid
      *dragGrid;  // The container currently being dragged from.

  static size_t useSlot;  // The slot whose item is currently being "used"
                          // (after right-clicking)
  static const ContainerGrid *useGrid;

  virtual void refresh() override;
  void refreshTooltip();

  static void leftMouseDown(Element &e, const ScreenPoint &mousePos);
  static void leftMouseUp(Element &e, const ScreenPoint &mousePos);
  static void rightMouseDown(Element &e, const ScreenPoint &mousePos);
  static void rightMouseUp(Element &e, const ScreenPoint &mousePos);
  static void mouseMove(Element &e, const ScreenPoint &mousePos);

  size_t getSlot(const ScreenPoint &mousePos) const;

 public:
  ContainerGrid(size_t rows, size_t cols, ClientItem::vect_t &linked,
                Serial serial = Serial::Inventory(), px_t x = 0, px_t y = 0,
                px_t gap = DEFAULT_GAP, bool solidBackground = true);
  static void cleanup();

  static const ClientItem *getDragItem();
  static const ClientItem *getUseItem();
  static void dropItem();  // Drop the item currently being dragged.

  static void clearUseItem();

  friend Client;
};

#endif
