#pragma once

#include "ClientObject.h"

class CDroppedItem : public ClientObject {
 public:
  class Type : public ClientObjectType {
   public:
    Type();
  };

  CDroppedItem(Client &client, Serial serial, const MapPoint &location,
               const ClientItem &itemType, size_t quantity, Hitpoints health,
               std::string suffix, bool isNew);
  virtual const std::string &name() const override;
  virtual const Texture &image() const override;
  virtual const Texture &getHighlightImage() const override;
  virtual const Tooltip &tooltip() const override;
  virtual void onLeftClick() override;
  virtual void onRightClick() override;
  virtual void update(double delta) override;
  virtual void draw() const override;
  virtual bool isFlat() const override;
  virtual Color nameColor() const override;
  virtual bool obstructsConstruction() const override { return true; }

 private:
  const ClientItem &_itemType;
  size_t _quantity;
  Hitpoints _health{0};
  std::string _suffix;
  mutable std::string _name;
  double _altitude{0};
  double _fallSpeed{0};

  bool isFalling() const;

  static const double DROP_HEIGHT;
  static const double DROP_ACCELERATION;  // px/s/s
};
