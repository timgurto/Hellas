// (C) 2016 Tim Gurto

#include "ClientMerchantSlot.h"

ClientMerchantSlot::ClientMerchantSlot(const ClientItem *wareItemArg, size_t wareQtyArg,
                                 const ClientItem *priceItemArg, size_t priceQtyArg):
wareItem(wareItemArg),
priceItem(priceItemArg),
wareQty(wareQtyArg),
priceQty(priceQtyArg)
{}

ClientMerchantSlot::operator bool() const{
    return
        wareItem != nullptr &&
        priceItem != nullptr &&
        wareQty > 0 &&
        priceQty > 0;
}

const ClientItemSet ClientMerchantSlot::ware() const{
    ClientItemSet is;
    is.add(wareItem, wareQty);
    return is;
}

const ClientItemSet ClientMerchantSlot::price() const{
    ClientItemSet is;
    is.add(priceItem, priceQty);
    return is;
}
