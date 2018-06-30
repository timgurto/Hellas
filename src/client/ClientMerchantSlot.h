#ifndef CLIENT_MERCHANT_SLOT_H
#define CLIENT_MERCHANT_SLOT_H

#include "../server/ItemSet.h"
#include "ClientItem.h"

struct ClientMerchantSlot {
  const ClientItem *wareItem, *priceItem;
  size_t wareQty, priceQty;

  ClientMerchantSlot(const ClientItem *wareItem = nullptr, size_t wareQty = 0,
                     const ClientItem *priceItem = nullptr,
                     size_t priceQty = 0);

  operator bool() const;

  const ItemSet ware() const;
  const ItemSet price() const;
};

#endif
