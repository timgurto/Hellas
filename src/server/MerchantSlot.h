// (C) 2016 Tim Gurto

#ifndef MERCHANT_SLOT_H
#define MERCHANT_SLOT_H

#include "ItemSet.h"

class ServerItem;

struct MerchantSlot{
    const ServerItem
        *wareItem,
        *priceItem;
    size_t
        wareQty,
        priceQty;

    MerchantSlot(const ServerItem *wareItem = nullptr, size_t wareQty = 0,
                 const ServerItem *priceItem = nullptr, size_t priceQty = 0);

    operator bool() const;
    
    const ItemSet ware() const;
    const ItemSet price() const;
};

#endif
