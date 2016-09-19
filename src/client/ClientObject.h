// (C) 2015-2016 Tim Gurto

#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include <sstream>
#include <string>

#include "ClientObjectType.h"
#include "Entity.h"
#include "ClientItem.h"
#include "../Point.h"
#include "../Server/MerchantSlot.h"

class Element;
struct MerchantSlot;
class TextBox;
class Window;

// A client-side description of an object
class ClientObject : public Entity{
    size_t _serial;
    std::string _owner;
    ClientItem::vect_t _container;
    Window *_window; // For containers, etc.
    std::vector<MerchantSlot> _merchantSlots;
    // Used for either the trade screen, or the merchant setup screen.
    std::vector<Element *> _merchantSlotElements;
    typedef std::pair<size_t, size_t> serialSlotPair_t;
    std::vector<serialSlotPair_t *> _serialSlotPairs;
    std::vector<TextBox *> _wareQtyBoxes;
    std::vector<TextBox *> _priceQtyBoxes;

public:
    ClientObject(const ClientObject &rhs);
    // Serial only: create dummy object, for set searches
    ClientObject(size_t serial, const EntityType *type = nullptr, const Point &loc = Point());
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
    
    virtual void draw(const Client &client) const;

    Rect collisionRect() const { return objectType()->collisionRect() + location(); }

    virtual void onRightClick(Client &client);
    static void startDeconstructing(void *object);
    static void trade(void *serialAndSlot);
    static void sendMerchantSlot(void *serialAndSlot);
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const;

    void playGatherSound() const;

    virtual bool isObject(){ return true; }

    void refreshWindow();
    void hideWindow();


    bool userHasAccess() const;

    void setMerchantSlot(size_t i, MerchantSlot &mSlot);
};

#endif
