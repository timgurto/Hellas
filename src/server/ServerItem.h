#ifndef SERVER_ITEM_H
#define SERVER_ITEM_H

#include <SDL.h>

#include <map>

#include "../Item.h"
#include "ItemSet.h"

class ObjectType;

// Describes an item type
class ServerItem : public Item {
  size_t _stackSize = 1;

  // The object that this item can construct
  const ObjectType *_constructsObject;

  // An item returned to the user after this is used as a construction material
  const ServerItem *_returnsOnConstruction = nullptr;

  bool _loaded{false};

  std::string _exclusiveToQuest{};

 public:
  ServerItem(const std::string &id);

  typedef std::pair<const ServerItem *, size_t> Slot;
  typedef std::vector<Slot> vect_t;

  size_t stackSize() const { return _stackSize; }
  void stackSize(size_t n) { _stackSize = n; }
  void constructsObject(const ObjectType *obj) { _constructsObject = obj; }
  const ObjectType *constructsObject() const { return _constructsObject; }
  const ServerItem *returnsOnConstruction() const {
    return _returnsOnConstruction;
  }
  void returnsOnConstruction(const ServerItem *item) {
    _returnsOnConstruction = item;
  }
  void exclusiveToQuest(const std::string &id) { _exclusiveToQuest = id; }
  bool isQuestExclusive() const { return !_exclusiveToQuest.empty(); }
  const std::string &exclusiveToQuest() const { return _exclusiveToQuest; }
  bool valid() const { return _loaded; }
  void loaded() { _loaded = true; }

  void fetchAmmoItem() const override;
};

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item,
                  size_t qty = 1);
bool vectHasSpaceAfterRemovingItems(const ServerItem::vect_t &vect,
                                    const ServerItem *item, size_t qty,
                                    const ServerItem *itemThatWillBeRemoved,
                                    size_t qtyThatWillBeRemoved);

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ItemSet &itemSet, const ServerItem::vect_t &vect);
bool operator>(const ServerItem::vect_t &vect, const ItemSet &itemSet);

const ServerItem *toServerItem(const Item *item);

#endif
