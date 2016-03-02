// (C) 2016 Tim Gurto

#ifndef MERCHANT_SLOT
#define MERCHANT_SLOT

class Item;

struct MerchantSlot{
    const Item *ware, *price;
    size_t wareQty, priceQty;

    MerchantSlot(const Item *ware = 0, size_t wareQty = 0,
                 const Item *price = 0, size_t priceQty = 0);

    operator bool() const;
};

#endif
