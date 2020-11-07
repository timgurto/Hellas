#ifndef MERCHANT_SLOT_H
#define MERCHANT_SLOT_H

#include "ItemSet.h"

class Item;

struct MerchantSlot {
  const Item *wareItem, *priceItem;
  size_t wareQty, priceQty;

  MerchantSlot(const Item *wareItem = nullptr, size_t wareQty = 0,
               const Item *priceItem = nullptr, size_t priceQty = 0);

  operator bool() const;

  ItemSet ware() const;
  ItemSet price() const;
};

#endif
