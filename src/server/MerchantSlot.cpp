// (C) 2016 Tim Gurto

#include "MerchantSlot.h"

MerchantSlot::MerchantSlot(const Item *wareItem, size_t wareQty,
                           const Item *priceItem, size_t priceQty):
_wareItem(wareItem),
_priceItem(priceItem),
_wareQty(wareQty),
_priceQty(priceQty)
{
    _ware.add(_wareItem, _wareQty);
    _price.add(_priceItem, _priceQty);
}

MerchantSlot::operator bool() const{
    return
        _wareItem &&
        _priceItem &&
        _wareQty > 0 &&
        _priceQty > 0;
}

void MerchantSlot::wareItem(const Item *item){
    _wareItem = item;
    _ware = ItemSet();
    _ware.add(_wareItem, _wareQty);
}

void MerchantSlot::wareQty(size_t qty){
    _wareQty = qty;
    _ware.set(_wareItem, qty);
}

void MerchantSlot::priceItem(const Item *item){
    _priceItem = item;
    _price = ItemSet();
    _price.add(_priceItem, _priceQty);
}

void MerchantSlot::priceQty(size_t qty){
    _priceQty = qty;
    _price.set(_priceItem, qty);
}
