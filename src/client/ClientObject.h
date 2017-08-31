#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include <sstream>
#include <string>

#include "ClientCombatant.h"
#include "ClientMerchantSlot.h"
#include "ClientObjectType.h"
#include "Sprite.h"
#include "ClientItem.h"
#include "../Point.h"

class Element;
struct MerchantSlot;
class TakeContainer;
class TextBox;
class Window;

// A client-side description of an object
class ClientObject : public Sprite, public ClientCombatant {
    size_t _serial;
    std::string _owner;
    ClientItem::vect_t _container;
    std::vector<ClientMerchantSlot> _merchantSlots;
    // Used for either the trade screen, or the merchant setup screen.
    std::vector<Element *> _merchantSlotElements;
    typedef std::pair<size_t, size_t> serialSlotPair_t;
    std::vector<serialSlotPair_t *> _serialSlotPairs;
    std::vector<TextBox *> _wareQtyBoxes;
    std::vector<TextBox *> _priceQtyBoxes;
    bool _beingGathered; // For aesthetic effects
    ItemSet _constructionMaterials; // The required materials, if it's being constructed.
    ClientItem::vect_t _dropbox; // A slot where construction materials can be added.
    ms_t _transformTimer;
    ms_t _gatherSoundTimer;

    /*
    True if the NPC is dead and has loot available.  This is used as the loot itself is unknown
    until a user opens the container.
    */
    bool _lootable;
    TakeContainer *_lootContainer;

    TextBox *_actionTextEntry = nullptr;

protected:
    Window *_window; // For containers, etc; opens when the object is nearby and right-clicked.
    ConfirmationWindow *_confirmCedeWindow;

public:
    ClientObject(const ClientObject &rhs);
    // Serial only: create dummy object, for set searches
    ClientObject(size_t serial, const ClientObjectType *type = nullptr, const Point &loc = Point());
    virtual ~ClientObject();

    bool operator<(const ClientObject &rhs) const { return _serial < rhs._serial; }
    bool operator==(const ClientObject &rhs) const { return _serial == rhs._serial; }

    size_t serial() const { return _serial; }
    const ClientObjectType *objectType() const
        { return dynamic_cast<const ClientObjectType *>(type()); }
    const std::string &owner() const { return _owner; }
    void owner(const std::string &name) { _owner = name; }
    ClientItem::vect_t &container() { return _container; }
    const ClientItem::vect_t &container() const { return _container; }
    const std::vector<Element *> &merchantSlotElements() const
        { return _merchantSlotElements; }
    const Window *window() const { return _window; }
    void beingGathered(bool b) { _beingGathered = b; _gatherSoundTimer = 0; }
    bool beingGathered() const { return _beingGathered; }
    const ItemSet &constructionMaterials() const { return _constructionMaterials; }
    void constructionMaterials(const ItemSet &set) { _constructionMaterials = set; }
    bool isBeingConstructed() const { return !_constructionMaterials.isEmpty(); }
    void transformTimer(ms_t remaining) {_transformTimer = remaining; }
    bool lootable() const { return _lootable; }
    void lootable(bool b) { _lootable = b; }
    const TakeContainer *lootContainer() { return _lootContainer; }
    bool belongsToPlayer() const;
    bool belongsToPlayerCity() const;

    virtual char classTag() const override { return 'o'; }

    virtual void update(double delta) override;
    virtual const std::string &name() const override { return objectType()->name(); }
    virtual const Texture &image() const override;
    virtual const Texture &highlightImage() const override;
    const Texture &tooltip() const override; // Getter; creates tooltip on first call.

    Rect collisionRect() const { return objectType()->collisionRect() + location(); }

    virtual void onLeftClick(Client &client) override;
    virtual void onRightClick(Client &client) override;
    static void startDeconstructing(void *object);
    static void trade(void *serialAndSlot);
    static void sendMerchantSlot(void *serialAndSlot);

    virtual void onInventoryUpdate();
    void hideWindow();
    virtual void assembleWindow(Client &client);

    // From ClientCombatant
    virtual void sendTargetMessage() const override;
    virtual const Sprite *entityPointer() const override { return this; }
    virtual bool canBeAttackedByPlayer() const override;
    virtual const Point &combatantLocation() const { return location(); }
    virtual const Color &nameColor() const override;

    // From Sprite
    const Texture &cursor(const Client &client) const override;
    void draw(const Client &client) const;


    bool userHasAccess() const;
    bool canAlwaysSee() const;

    void setMerchantSlot(size_t i, ClientMerchantSlot &mSlot);
    

private:
    static const px_t
        BUTTON_HEIGHT,
        BUTTON_WIDTH,
        GAP,
        BUTTON_GAP;
    void addConstructionToWindow();
    void addMerchantSetupToWindow();
    void addInventoryToWindow();
    void addDeconstructionToWindow();
    void addVehicleToWindow();
    void addActionToWindow();
        static void performAction(void *object);
    void addMerchantTradeToWindow();
    void addCedeButtonToWindow();
        static void confirmAndCedeObject(void *objectToCede);
};

#endif
