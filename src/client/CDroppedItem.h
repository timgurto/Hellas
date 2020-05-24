#pragma once

#include "ClientObject.h"

class CDroppedItem : public ClientObject {
 public:
  class Type : public ClientObjectType {
   public:
    Type();
  };

  CDroppedItem(Serial serial, const MapPoint &location,
               const ClientItem &itemType, size_t quantity, bool isNew);
  virtual const std::string &name() const override;
  virtual const Texture &image() const override;
  virtual const Texture &getHighlightImage() const override;
  virtual const Tooltip &tooltip() const override;
  virtual void onLeftClick(Client &client) override;
  virtual void onRightClick(Client &client) override;
  virtual void update(double delta) override;
  virtual void draw(const Client &client) const override;
  virtual bool isFlat() const override;

 private:
  static Type commonType;

  const ClientItem &_itemType;
  size_t _quantity;
  mutable std::string _name;
  double _altitude{0};
  double _fallSpeed{0};

  static const double DROP_HEIGHT;
  static const double DROP_ACCELERATION;  // px/s/s
};
