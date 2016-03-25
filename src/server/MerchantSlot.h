// (C) 2016 Tim Gurto

#ifndef MERCHANT_SLOT_H
#define MERCHANT_SLOT_H

#include "ItemSet.h"

class Item;

class MerchantSlot{
    const Item
        *_wareItem,
        *_priceItem;
    size_t
        _wareQty,
        _priceQty;
    ItemSet
        _ware,
        _price;

public:
    MerchantSlot(const Item *wareItem = nullptr, size_t wareQty = 0,
                 const Item *priceItem = nullptr, size_t priceQty = 0);

    operator bool() const;
    
    const Item *wareItem() const { return _wareItem; };
    void wareItem(const Item *item);
    size_t wareQty() const { return _wareQty; };
    void wareQty(size_t qty);
    const Item *priceItem() const { return _priceItem; };
    void priceItem(const Item *item);
    size_t priceQty() const { return _priceQty; };
    void priceQty(size_t qty);
    const ItemSet &ware() const { return _ware; }
    const ItemSet &price() const { return _price; }
};

#endif
