// (C) 2016 Tim Gurto

#include "MerchantSlot.h"

MerchantSlot::MerchantSlot(const Item *ware, size_t wareQty, const Item *price, size_t priceQty):
ware(0),
price(0),
wareQty(0),
priceQty(0)
{}

MerchantSlot::operator bool() const{
    return
        ware &&
        price &&
        wareQty > 0 &&
        priceQty > 0;
}
