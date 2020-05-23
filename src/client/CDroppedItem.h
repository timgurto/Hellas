#pragma once

#include "ClientObject.h"

class CDroppedItem : public ClientObject {
 public:
  class Type : public ClientObjectType {
   public:
    Type();
  };

  CDroppedItem(Serial serial, const MapPoint &location,
               const ClientItem &itemType);
  virtual const std::string &name() const override;

 private:
  static Type commonType;

  const ClientItem &_itemType;
};
