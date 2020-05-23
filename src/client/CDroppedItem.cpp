#include "CDroppedItem.h"

CDroppedItem::Type CDroppedItem::commonType;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {}

CDroppedItem::CDroppedItem(Serial serial, const MapPoint& location,
                           const ClientItem& itemType)
    : ClientObject(serial, &commonType, location), _itemType(itemType) {}

const std::string& CDroppedItem::name() const { return _itemType.name(); }
