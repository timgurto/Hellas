#include "MerchantSlot.h"

MerchantSlot::MerchantSlot(const Item *wareItemArg, size_t wareQtyArg,
                           const Item *priceItemArg, size_t priceQtyArg)
    : wareItem(wareItemArg),
      priceItem(priceItemArg),
      wareQty(wareQtyArg),
      priceQty(priceQtyArg) {}

MerchantSlot::operator bool() const {
  return wareItem != nullptr && priceItem != nullptr && wareQty > 0 &&
         priceQty > 0;
}

ItemSet MerchantSlot::ware() const {
  ItemSet is;
  is.add(wareItem, wareQty);
  return is;
}

ItemSet MerchantSlot::price() const {
  ItemSet is;
  is.add(priceItem, priceQty);
  return is;
}
