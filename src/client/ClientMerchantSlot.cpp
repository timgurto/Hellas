#include "ClientMerchantSlot.h"

ClientMerchantSlot::ClientMerchantSlot(const ClientItem *wareItemArg,
                                       size_t wareQtyArg,
                                       const ClientItem *priceItemArg,
                                       size_t priceQtyArg)
    : wareItem(wareItemArg),
      priceItem(priceItemArg),
      wareQty(wareQtyArg),
      priceQty(priceQtyArg) {}

ClientMerchantSlot::operator bool() const {
  return wareItem != nullptr && priceItem != nullptr && wareQty > 0 &&
         priceQty > 0;
}

const ItemSet ClientMerchantSlot::ware() const {
  ItemSet is;
  is.add(wareItem, wareQty);
  return is;
}

const ItemSet ClientMerchantSlot::price() const {
  ItemSet is;
  is.add(priceItem, priceQty);
  return is;
}
