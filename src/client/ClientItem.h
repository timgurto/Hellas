#ifndef CLIENT_ITEM_H
#define CLIENT_ITEM_H

#include <map>

#include "../Item.h"
#include "../Optional.h"
#include "HasSounds.h"
#include "Projectile.h"
#include "Texture.h"
#include "Tooltip.h"

class ClientObjectType;
class SoundProfile;

// The client-side representation of an item type
class ClientItem : public Item, public HasSounds {
 public:
  class Instance {
   public:
    Instance() = default;
    Instance(const ClientItem *type, Hitpoints health, bool isSoulbound,
             std::string suffix)
        : _type(type),
          _health(health),
          _isSoulbound(isSoulbound),
          _suffix(suffix) {}

    const ClientItem *type() const { return _type; }
    Hitpoints health() const { return _health; }
    const Tooltip &tooltip() const;  // Return the appropriate tooltip,
                                     // generating it first if appropriate.
    bool isSoulbound() const;
    std::string name() const;
    bool isDamaged() const;
    bool shouldDrawAsBroken() const;
    bool shouldDrawAsDamaged() const;

   private:
    const ClientItem *_type{nullptr};
    Hitpoints _health{0};
    bool _isSoulbound{false};
    std::string _suffix{};

    mutable Optional<Tooltip> _tooltip;  // Builds on the basic item tooltip
    void createRegularTooltip() const;
    mutable Optional<Tooltip> _repairTooltip;
    void createRepairTooltip() const;
  };

 private:
  std::string _name;
  ImageWithHighlight _icon;
  Texture _gearImage;
  ScreenPoint _drawLoc;
  bool _isQuestItem{false};
  std::string _suffixSet{};
  mutable std::string _suffixInGeneratedTooltip{};

  // Should never be null.  Pointer rather than a reference so
  // that the type can be copied.
  const Client *_client{nullptr};

  mutable Optional<Tooltip> _tooltip;
  const Projectile::Type *_projectile{nullptr};

  struct Particles {
    std::string profile;
    MapPoint offset;  // Relative to gear offset for that slot.
  };
  std::vector<Particles> _particles;

  // The object that this item can construct
  const ClientObjectType *_constructsObject;

  static std::map<int, size_t> gearDrawOrder;
  static std::vector<ScreenPoint> gearOffsets;

 public:
  ClientItem();  // Needed for containers
  ClientItem(const Client &client, const std::string &id,
             const std::string &name);

  bool operator<(const ClientItem &rhs) const { return _name < rhs._name; }

  const Client &client() const { return *_client; }

  const std::string &name() const { return _name; }
  std::string nameWithSuffix(std::string suffixID) const;
  const StatsMod &getSuffixStats(std::string suffixID) const;
  const Texture &icon() const { return _icon.getNormalImage(); }
  const Texture &iconHighlighted() const { return _icon.getHighlightImage(); }
  void icon(const std::string &filename);
  void gearImage(const std::string &filename);
  void drawLoc(const ScreenPoint &loc) { _drawLoc = loc; }
  static const std::map<int, size_t> &drawOrder() { return gearDrawOrder; }
  void projectile(const Projectile::Type *p) { _projectile = p; }
  const Projectile::Type *projectile() const { return _projectile; }
  bool canUse() const;
  bool shouldWarnBeforeScrapping() const;
  Color nameColor() const { return qualityColor(_quality); }
  static Color qualityColor(Quality q);
  static std::string qualityName(Quality q);
  bool canBeScrapped() const;
  void markAsQuestItem() { _isQuestItem = true; }
  bool isQuestItem() const { return _isQuestItem; }
  void setSuffixSet(std::string suffixSetID) { _suffixSet = suffixSetID; }

  static const ScreenPoint &gearOffset(size_t slot) {
    return gearOffsets[slot];
  }

  void fetchAmmoItem() const override;

  void addParticles(const std::string &profileName, const MapPoint &offset);
  const std::vector<Particles> &particles() const { return _particles; }

  typedef std::vector<std::pair<ClientItem::Instance, size_t> > vect_t;

  void constructsObject(const ClientObjectType *obj) {
    _constructsObject = obj;
  }
  const ClientObjectType *constructsObject() const { return _constructsObject; }

  const Tooltip &tooltip(std::string suffixID = {}) const;
  void refreshTooltip() { _tooltip = Optional<Tooltip>{}; }

  enum DrawOffset { MAP_OFFSET, NO_OFFSET };
  void draw(const ScreenPoint &screenLoc) const;

  static void init();

  struct CompareName {
    bool operator()(const ClientItem *lhs, const ClientItem *rhs);
  };
};

const ClientItem *toClientItem(const Item *item);

class ClientItemAlphabetical {
 public:
  ClientItemAlphabetical(const ClientItem *item) : p(item) {}

  bool operator<(const ClientItemAlphabetical &rhs) const {
    return p->name() < rhs.p->name();
  }

  const ClientItem *operator->() const { return p; }

 private:
  const ClientItem *p;
};

#endif
