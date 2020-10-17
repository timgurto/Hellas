#ifndef AVATAR_H
#define AVATAR_H

#include <string>

#include "../Point.h"
#include "ClassInfo.h"
#include "ClientCombatant.h"
#include "ClientCombatantType.h"
#include "ClientItem.h"
#include "ClientVehicle.h"
#include "Sprite.h"

class CRecipe;

// The client-side representation of a user, including the player
class Avatar : public Sprite, public ClientCombatant {
 public:
  static const MapRect COLLISION_RECT;
  static const ScreenRect DRAW_RECT;

 private:
  std::string _name;
  const ClassInfo *_class = nullptr;
  std::string _city;
  ClientItem::vect_t _gear;
  bool _isKing = false;
  ClientVehicle *_vehicle{nullptr};
  mutable Texture _imageWithGear, _highlightImageWithGear;
  px_t _pixelsToCutOffBottomWhenDrawn{0};  // For drawing inside a vehicle

  const CRecipe *_currentlyCrafting{nullptr};
  ms_t _craftingSoundTimer{0};

 public:
  Avatar(const std::string &name, const MapPoint &location, Client &client);

  const MapRect collisionRect() const { return COLLISION_RECT + location(); }
  static const MapRect &collisionRectRaw() { return COLLISION_RECT; }
  void setClass(const ClassInfo::Name &newClass);
  const ClassInfo *getClass() const { return _class; }
  const ClientItem::vect_t &gear() const { return _gear; }
  ClientItem::vect_t &gear() { return _gear; }
  void driving(ClientVehicle &v) { _vehicle = &v; }
  void notDriving() { _vehicle = nullptr; }
  bool isDriving() const { return _vehicle != nullptr; }
  bool isDriving(const ClientVehicle &v) const { return _vehicle == &v; }
  const ClientVehicle *vehicle() const { return _vehicle; }
  double vehicleSpeed() const;
  const ClientItem *getRandomArmor() const {
    return _gear[Item::getRandomArmorSlot()].first.type();
  }
  void cityName(const std::string &name) { _city = name; }
  const std::string &cityName() const { return _city; }
  bool isInPlayersCity() const;
  void setAsKing() { _isKing = true; }
  bool isKing() const { return _isKing; }
  void levelUp() { level(level() + 1); }
  void cutOffBottomWhenDrawn(px_t numRows) {
    _pixelsToCutOffBottomWhenDrawn = numRows;
  }
  void startCrafting(const CRecipe &recipe) { _currentlyCrafting = &recipe; }
  void stopCrafting() { _currentlyCrafting = nullptr; }

  // From Sprite
  void draw() const override;
  void drawName() const override;
  void update(double delta) override;
  const Tooltip &tooltip()
      const override;  // Getter; creates tooltip on first call.
  void onLeftClick() override;
  void onRightClick() override;
  const std::string &name() const override { return _name; }
  const Texture &cursor() const override;
  void name(const std::string &newName) { _name = newName; }
  bool shouldDrawName() const override { return true; }
  Color nameColor() const override;
  virtual const Texture &image() const override { return _imageWithGear; }
  virtual const Texture &getHighlightImage() const override {
    return _highlightImageWithGear;
  }
  double speed() const override;
  virtual void onNewLocationFromServer() override;
  virtual void onLocationChange() override;

  // From ClientCombatant
  void sendTargetMessage() const override;
  void sendSelectMessage() const override;
  bool canBeAttackedByPlayer() const override;
  const Sprite *entityPointer() const override { return this; }
  const MapPoint &combatantLocation() const override { return location(); }
  const Color &healthBarColor() const override { return nameColor(); }

  void addMenuButtons(List &menu) override;

  void playAttackSound() const override;
  void playDefendSound() const override;
  void playDeathSound() const override;

  friend class Client;
};

#endif
