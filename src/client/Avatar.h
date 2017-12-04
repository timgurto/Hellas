#ifndef AVATAR_H
#define AVATAR_H

#include <string>

#include "ClassInfo.h"
#include "ClientItem.h"
#include "Sprite.h"
#include "ClientCombatant.h"
#include "ClientCombatantType.h"
#include "../Point.h"

// The client-side representation of a user, including the player
class Avatar : public Sprite, public ClientCombatant{
    static ClientCombatantType _combatantType;
    static SpriteType _spriteType;
    static const MapRect COLLISION_RECT;
    static const ScreenRect DRAW_RECT;

    MapPoint _destination;
    std::string _name;
    const ClassInfo *_class = nullptr;
    std::string _city;
    ClientItem::vect_t _gear;
    bool _isKing = false;
    bool _driving;

public:
    Avatar(const std::string &name, const MapPoint &location);

    const MapPoint &destination() const { return _destination; }
    void destination(const MapPoint &dst) { _destination = dst; }
    const MapRect collisionRect() const { return COLLISION_RECT + location(); }
    static const MapRect &collisionRectRaw() { return COLLISION_RECT; }
    void setClass(const ClassInfo::Name &newClass);
    const ClassInfo &getClass() const { assert(_class);  return *_class; }
    const ClientItem::vect_t &gear() const { return _gear; }
    ClientItem::vect_t &gear() { return _gear; }
    void driving(bool b) { _driving = b; }
    bool isDriving() const { return _driving; }
    const ClientItem *getRandomArmor() const { return _gear[Item::getRandomArmorSlot()].first; }
    void cityName(const std::string &name) { _city = name; }
    const std::string &cityName() const { return _city; }
    bool isInPlayersCity() const;
    void setAsKing() { _isKing = true; }
    bool isKing() const { return _isKing; }
    void levelUp() { level( level() + 1); }

    // From Sprite
    void draw(const Client &client) const override;
    void drawName() const override;
    void update(double delta) override;
    const Texture &tooltip() const override; // Getter; creates tooltip on first call.
    void onLeftClick(Client &client) override;
    void onRightClick(Client &client) override;
    const std::string &name() const override { return _name; }
    const Texture &cursor(const Client &client) const override;
    void name(const std::string &newName) { _name = newName; }
    bool shouldDrawName() const override { return true; }
    const Color &nameColor() const override;
    virtual const Texture &image() const override { assert(_class);  return _class->image(); }
    virtual const Texture &highlightImage() const override { return _class->image(); }

    // From ClientCombatant
    void sendTargetMessage() const override;
    void sendSelectMessage() const override;
    bool canBeAttackedByPlayer() const override;
    const Sprite *entityPointer() const override { return this; }
    const MapPoint &combatantLocation() const override { return location(); }
    bool shouldDrawHealthBar() const override;
    const Color &healthBarColor() const override { return nameColor(); }

    void addMenuButtons(List &menu) const override;
    static void declareWarAgainstPlayer(void *pUsername);
    static void declareWarAgainstCity(void *pCityName);
    static void recruit(void *pUsername);

    void playAttackSound() const; // The player has attacked; play an appropriate sound.
    void playDefendSound() const; // The player has been attacked; play an appropriate sound.

    friend class Client;

private:
    /*
    Get the next location towards destination, with distance determined by
    this client's latency, and by time elapsed.
    This is used to smooth the apparent movement of other users.
    */
    MapPoint interpolatedLocation(double delta);
};

#endif
