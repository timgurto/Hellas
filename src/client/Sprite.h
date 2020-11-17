#ifndef SPRITE_H
#define SPRITE_H

#include <set>
#include <vector>

#include "../Optional.h"
#include "../Point.h"
#include "SpriteType.h"
#include "Texture.h"
#include "Tooltip.h"

class Client;
class SpriteType;

// Handles the graphical and UI side of in-game objects  Abstract class
class Sprite {
  bool _yChanged;  // y co-ordinate has changed, and the entity must be
                   // reordered.
  const SpriteType *_type;
  MapPoint _location;

  // Attack animation
  MapPoint _offsetForAttackAnimation;
  static const double ATTACK_ANIMATION_SPEED;  // px/s
  static const double ATTACK_ANIMATION_DISTANCE;
  void progressAttackAnimation(double delta);

  // Lerping
  MapPoint _locationOnServer;
  bool _serverHasOrderedACorrection{true};
  void driftTowardsServerLocation(double delta);

  bool _toRemove;  // No longer draw or update, and remove when possible.

  static const std::string EMPTY_NAME;

  static const ScreenPoint HIGHLIGHT_OFFSET;

 protected:
  mutable Optional<Tooltip> _tooltip;
  Client &_client;

 public:
  Sprite(const SpriteType *type, const MapPoint &location, Client &client);
  static Sprite YCoordOnly(double y);
  virtual ~Sprite() {}

  const MapPoint &location() const { return _location; }
  void location(const MapPoint &loc);  // yChanged() should be checked after
                                       // changing location.
  virtual void onLocationChange() {}

  virtual ScreenRect drawRect() const;
  px_t width() const { return _type->width(); }
  px_t height() const { return _type->height(); }
  bool yChanged() const { return _yChanged; }
  void yChanged(bool val) { _yChanged = val; }
  const SpriteType *type() const { return _type; }
  void type(const SpriteType *et) { _type = et; }
  virtual const Texture &image() const { return _type->image(); }
  virtual const Texture &getHighlightImage() const {
    return _type->getHighlightImage();
  }
  void markForRemoval() { _toRemove = true; }
  bool markedForRemoval() const { return _toRemove; }
  virtual bool isFlat() const { return _type->isFlat(); }
  virtual std::string additionalTextInName() const { return {}; }
  virtual bool shouldAddParticles() const { return true; }
  bool isCharacter() const;

  // Movement lerping
  const MapPoint &locationOnServer() const { return _locationOnServer; }
  void newLocationFromServer(const MapPoint &loc);
  virtual void onNewLocationFromServer() {}
  bool serverHasOrderedACorrection() const {
    return _serverHasOrderedACorrection;
  }
  virtual px_t apparentMovementSpeed() const { return 0; }
  virtual double speed() const;

  virtual char classTag() const { return 'e'; }

  virtual void draw() const;  // Includes name, but not health bar.
  void drawShadow() const;
  virtual void drawName() const;
  virtual void update(double delta);
  virtual void onLeftClick() {}
  virtual void onRightClick() {}
  virtual const Texture &cursor() const;
  virtual const Tooltip &tooltip() const;
  virtual bool shouldDrawName() const { return false; }
  virtual bool shouldDrawShadow() const { return true; }
  virtual const std::string &name() const { return EMPTY_NAME; }
  virtual Color nameColor() const { return Color::COMBATANT_NEUTRAL; }
  void refreshTooltip() const { _tooltip = Optional<Tooltip>{}; }

  double bottomEdge() const;
  bool collision(const MapPoint &p) const;
  bool mouseIsOverRealPixel(const MapPoint &p) const;

  void animateAttackingTowards(const Sprite &target);
  MapPoint drawLocation() const;

  struct ComparePointers {
    bool operator()(const Sprite *lhs, const Sprite *rhs) const {
      // 1. location
      double lhsBottom = lhs->bottomEdge(), rhsBottom = rhs->bottomEdge();
      if (lhsBottom != rhsBottom) return lhsBottom < rhsBottom;

      // 2. memory address (to ensure a unique ordering)
      return lhs < rhs;
    }
  };

  typedef std::set<Sprite *, ComparePointers> set_t;
};

#endif
