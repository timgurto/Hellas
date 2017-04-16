#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include <sstream>
#include <string>

#include "ClientMerchantSlot.h"
#include "ClientObjectType.h"
#include "Entity.h"
#include "ClientItem.h"
#include "../Point.h"

class Element;
struct MerchantSlot;
class TextBox;
class Window;

// A client-side description of an object
class ClientObject : public Entity{
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

protected:
    Window *_window; // For containers, etc; opens when the object is nearby and right-clicked.

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

    virtual char classTag() const override { return 'o'; }

    virtual void update(double delta) override;
    virtual const Texture &cursor(const Client &client) const override;
    const Texture &tooltip() const override; // Getter; creates tooltip on first call.

    Rect collisionRect() const { return objectType()->collisionRect() + location(); }

    virtual void onRightClick(Client &client) override;
    static void startDeconstructing(void *object);
    static void trade(void *serialAndSlot);
    static void sendMerchantSlot(void *serialAndSlot);

    virtual void onInventoryUpdate();
    void hideWindow();
    virtual void assembleWindow(Client &client);
    virtual const Texture &image() const override;
    virtual const Texture &highlightImage() const override;


    bool userHasAccess() const;

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
    void addMerchantTradeToWindow();
};

#endif
