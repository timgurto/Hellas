#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include <sstream>
#include <string>

#include "../Point.h"
#include "ClientCombatant.h"
#include "ClientItem.h"
#include "ClientMerchantSlot.h"
#include "ClientObjectType.h"
#include "Sprite.h"
#include "Tooltip.h"
#include "ui/ConfirmationWindow.h"
#include "ui/TextBox.h"

class Element;
struct MerchantSlot;
class TakeContainer;
class TextBox;
class Window;

class CQuest;

// A client-side description of an object
class ClientObject : public Sprite, public ClientCombatant {
 public:
  struct Owner {
    enum Type { PLAYER, CITY, ALL_HAVE_ACCESS, NO_ACCESS };
    Type type;
    std::string name;
    Owner(Type type, std::string name);
    bool operator==(Owner &rhs) const;
  };

  size_t _serial;
  Owner _owner;
  ClientItem::vect_t _container;
  std::vector<ClientMerchantSlot> _merchantSlots;
  // Used for either the trade screen, or the merchant setup screen.
  std::vector<Element *> _merchantSlotElements;
  std::vector<TextBox *> _wareQtyBoxes;
  std::vector<TextBox *> _priceQtyBoxes;
  bool _beingGathered;             // For aesthetic effects
  ItemSet _constructionMaterials;  // The required materials, if it's being
                                   // constructed.
  ClientItem::vect_t
      _dropbox;  // A slot where construction materials can be added.
  ms_t _transformTimer;
  ms_t _gatherSoundTimer;

  /*
  True if the NPC is dead and has loot available.  This is used as the loot
  itself is unknown until a user opens the container.
  */
  bool _lootable;
  TakeContainer *_lootContainer;

  TextBox *_actionTextEntry = nullptr;

  void createRegularTooltip() const;
  virtual bool addClassSpecificStuffToTooltip(Tooltip &tooltip) const;
  mutable Optional<Tooltip> _repairTooltip;
  void createRepairTooltip() const;

 protected:
  Window *_window;  // For containers, etc; opens when the object is nearby and
                    // right-clicked.
  ConfirmationWindow *_confirmCedeWindow;
  ConfirmationWindow *_confirmDemolishWindow = nullptr;
  InputWindow *_grantWindow = nullptr;

  virtual std::string demolishVerb() const { return "demolish"; }
  virtual std::string demolishButtonText() const { return "Demolish"; }
  virtual std::string demolishButtonTooltip() const {
    return "Demolish this object, removing it permanently";
  }

 public:
  ClientObject(const ClientObject &rhs);
  // Serial only: create dummy object, for set searches
  ClientObject(size_t serial, const ClientObjectType *type = nullptr,
               const MapPoint &loc = MapPoint{});
  virtual ~ClientObject();

  bool operator<(const ClientObject &rhs) const {
    return _serial < rhs._serial;
  }
  bool operator==(const ClientObject &rhs) const {
    return _serial == rhs._serial;
  }

  size_t serial() const { return _serial; }
  const ClientObjectType *objectType() const {
    return dynamic_cast<const ClientObjectType *>(type());
  }
  const Owner &owner() const { return _owner; }
  void owner(const Owner &rhs) { _owner = rhs; }
  bool isOwnedByPlayer(const std::string &playerName) const;
  bool isOwnedByCity(const std::string &cityName) const;
  ClientItem::vect_t &container() { return _container; }
  const ClientItem::vect_t &container() const { return _container; }
  const std::vector<Element *> &merchantSlotElements() const {
    return _merchantSlotElements;
  }
  const Window *window() const { return _window; }
  void beingGathered(bool b) {
    _beingGathered = b;
    _gatherSoundTimer = 0;
  }
  bool beingGathered() const { return _beingGathered; }
  const ItemSet &constructionMaterials() const {
    return _constructionMaterials;
  }
  void constructionMaterials(const ItemSet &set) {
    _constructionMaterials = set;
  }
  bool isBeingConstructed() const { return !_constructionMaterials.isEmpty(); }
  void transformTimer(ms_t remaining) { _transformTimer = remaining; }
  bool lootable() const { return _lootable; }
  void lootable(bool b) { _lootable = b; }
  bool containerIsEmpty() const;
  const TakeContainer *lootContainer() const { return _lootContainer; }
  bool belongsToPlayer() const;
  bool belongsToPlayerCity() const;
  std::string additionalTextInName() const override;

  virtual char classTag() const override { return 'o'; }

  virtual void update(double delta) override;
  virtual const std::string &name() const override {
    return objectType()->name();
  }
  virtual const Texture &image() const override;
  virtual const Texture &highlightImage() const override;
  const Tooltip &tooltip()
      const override;  // Getter; creates tooltip on first call.

  MapRect collisionRect() const {
    return objectType()->collisionRect() + location();
  }
  bool collides() const { return objectType()->collides(); }

  virtual void onLeftClick(Client &client) override;
  virtual void onRightClick(Client &client) override;
  static void startDeconstructing(void *object);
  static void trade(size_t serial, size_t slot);
  static void sendMerchantSlot(size_t serial, size_t slot);

  virtual void onInventoryUpdate();
  void hideWindow();
  virtual void assembleWindow(Client &client);

  // Quests
  std::set<CQuest *> startsQuests() const;
  std::set<CQuest *> completableQuests() const;

  // From ClientCombatant
  virtual void sendTargetMessage() const override;
  virtual void sendSelectMessage() const override;
  virtual const Sprite *entityPointer() const override { return this; }
  virtual bool canBeAttackedByPlayer() const override;
  virtual const MapPoint &combatantLocation() const { return location(); }
  const Color &healthBarColor() const override { return nameColor(); }
  void playAttackSound() const override;
  void playDefendSound() const override;
  void playDeathSound() const override;

  // From Sprite
  void draw(const Client &client) const override;
  void drawAppropriateQuestIndicator() const;
  const Texture &cursor(const Client &client) const override;
  virtual const Color &nameColor() const override;
  virtual bool shouldDrawName() const override;
  virtual bool shouldDrawShadow() const override;
  bool shouldAddParticles() const override {
    return isAlive() && !isBeingConstructed();
  }
  bool isFlat() const override;

  bool userHasAccess() const;
  bool userHasMerchantAccess() const;
  bool canAlwaysSee() const;

  void setMerchantSlot(size_t i, ClientMerchantSlot &mSlot);

 protected:
  static const px_t BUTTON_HEIGHT, BUTTON_WIDTH, GAP, BUTTON_GAP;

 private:
  void addQuestsToWindow();
  void addConstructionToWindow();
  void addMerchantSetupToWindow();
  void addInventoryToWindow();
  void addDeconstructionToWindow();
  void addActionToWindow();
  static void performAction(void *object);
  void addMerchantTradeToWindow();
  void addCedeButtonToWindow();
  static void confirmAndCedeObject(void *objectToCede);
  void addGrantButtonToWindow();
  static void getInputAndGrantObject(void *objectToGrant);
  void addDemolishButtonToWindow();
  static void confirmAndDemolishObject(void *objectToDemolish);

  // Return value: whether anything was added
  virtual bool addClassSpecificStuffToWindow() { return false; }
};

#endif
