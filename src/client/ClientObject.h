// (C) 2015 Tim Gurto

#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include <SDL_mixer.h>
#include <sstream>
#include <string>

#include "ClientObjectType.h"
#include "Entity.h"
#include "Item.h"
#include "../Point.h"
#include "../server/MerchantSlot.h"

class Element;
class Window;

// A client-side description of an object
class ClientObject : public Entity{
    size_t _serial;
    std::string _owner;
    Item::vect_t _container;
    Window *_window; // For containers, etc.
    std::vector<MerchantSlot> _merchantSlots;
    std::vector<Element *> _merchantSlotElements;

public:
    ClientObject(const ClientObject &rhs);
    // Serial only: create dummy object, for set searches
    ClientObject(size_t serial, const EntityType *type = 0, const Point &loc = Point());
    virtual ~ClientObject();

    bool operator<(const ClientObject &rhs) const { return _serial < rhs._serial; }
    bool operator==(const ClientObject &rhs) const { return _serial == rhs._serial; }

    size_t serial() const { return _serial; }
    const ClientObjectType *objectType() const
        { return dynamic_cast<const ClientObjectType *>(type()); }
    const std::string &owner() const { return _owner; }
    void owner(const std::string &name) { _owner = name; }
    Item::vect_t &container() { return _container; }
    const Item::vect_t &container() const { return _container; }
    
    virtual void draw(const Client &client) const;

    Rect collisionRect() const { return objectType()->collisionRect() + location(); }

    virtual void onRightClick(Client &client);
    static void startDeconstructing(void *object);
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const;

    void playGatherSound() const;

    virtual bool isObject(){ return true; }

    void refreshWindow();

    bool userHasAccess() const;

    void setMerchantSlot(size_t i, const MerchantSlot &mSlot);
};

#endif
