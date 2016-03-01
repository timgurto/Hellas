// (C) 2016 Tim Gurto

#ifndef MERCHANT_SLOT
#define MERCHANT_SLOT

class Item;

struct MerchantSlot{
    const Item *ware, *price;
    size_t wareQty, priceQty;

    MerchantSlot();
};

#endif
