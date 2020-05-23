#include "CDroppedItem.h"

CDroppedItem::Type CDroppedItem::commonType;

CDroppedItem::Type::Type() : ClientObjectType("droppedItem") {}

CDroppedItem::CDroppedItem(const ClientItem& itemType)
    : ClientObject({}, &commonType, {}), _itemType(itemType) {}

const std::string& CDroppedItem::name() const { return _itemType.name(); }
