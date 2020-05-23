#include "CDroppedItem.h"

CDroppedItem::Type CDroppedItem::commonType;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {}

CDroppedItem::CDroppedItem(Serial serial, const ClientItem& itemType)
    : ClientObject(serial, &commonType, {}), _itemType(itemType) {}

const std::string& CDroppedItem::name() const { return _itemType.name(); }
