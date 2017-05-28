#include "Avatar.h"
#include "Client.h"
#include "Renderer.h"
#include "SoundProfile.h"
#include "TooltipBuilder.h"
#include "../util.h"

extern Renderer renderer;

const Rect Avatar::DRAW_RECT(-9, -39, 20, 40);
const Rect Avatar::COLLISION_RECT(-5, -2, 10, 4);
std::map<std::string, SpriteType> Avatar::_classes;
ClientCombatantType Avatar::_combatantType(Client::MAX_PLAYER_HEALTH);

Avatar::Avatar(const std::string &name, const Point &location):
Sprite(&_classes[""], location),
ClientCombatant(&_combatantType),
_name(name),
_gear(Client::GEAR_SLOTS, std::make_pair(nullptr, 0)),
_driving(false){}

Point Avatar::interpolatedLocation(double delta){
    if (_destination == location())
        return _destination;;

    const double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(location(), _destination, maxLegalDistance);
}

void Avatar::draw(const Client &client) const{
    if (_driving)
        return;

    Sprite::draw(client);
    
    // Draw gear
    for (const auto &pair : ClientItem::drawOrder()){
        const ClientItem *item = _gear[pair.second].first;
        if (item != nullptr)
            item->draw(location());
    }

    if (isDebug()) {
        renderer.setDrawColor(Color::WHITE);
        renderer.drawRect(COLLISION_RECT + location() + client.offset());
    }

    // Draw username
    if (_name != client.username()) {
        const Client &client = *Client::_instance;
        bool isAtWar = client.isAtWarWith(_name);

        const Texture nameLabel(client.defaultFont(), _name, nameColor());
        const Texture nameOutline(client.defaultFont(), _name, Color::PLAYER_NAME_OUTLINE);
        Texture cityOutline, cityLabel;

        Point namePosition = location() + client.offset();
        namePosition.y -= 60;
        namePosition.x -= nameLabel.width() / 2;

        Point cityPosition;
        bool shouldDrawCityName = ! _city.empty();
        if (shouldDrawCityName){
            std::string cityText = "of " + _city;
            cityOutline = Texture(client.defaultFont(), cityText, Color::PLAYER_NAME_OUTLINE);
            cityLabel = Texture(client.defaultFont(), cityText, nameColor());
            cityPosition.x = location().x + client.offset().x - cityLabel.width() / 2;
            cityPosition.y = namePosition.y;
            namePosition.y -= 11;
        }
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y){
                nameOutline.draw(namePosition + Point(x, y));
                if (shouldDrawCityName)
                    cityOutline.draw(cityPosition + Point(x, y));
            }
        nameLabel.draw(namePosition);
        if (shouldDrawCityName)
            cityLabel.draw(cityPosition);
    }

    drawHealthBarIfAppropriate(location(), height());
}

void Avatar::update(double delta){
    location(interpolatedLocation(delta));
}

void Avatar::cleanup(){
    _classes.clear();
}

void Avatar::setClass(const std::string &c){
    _class = c;
    if (_classes.find(c) == _classes.end())
        _classes[c] = SpriteType(DRAW_RECT, std::string("Images/" + c + ".png"));
    type(&_classes[c]);
}

const Texture &Avatar::tooltip() const{
    if (_tooltip)
        return _tooltip;

    // Name
    TooltipBuilder tb;
    tb.setColor(Color::ITEM_NAME);
    tb.addLine(_name);

    // Class
    tb.addGap();
    tb.setColor(Color::ITEM_TAGS);
    tb.addLine(getClass());

    // Debug info
    /*if (isDebug()){
        tb.addGap();
        tb.setColor(Color::ITEM_TAGS);
        tb.addLine("");
    }*/

    _tooltip = tb.publish();
    return _tooltip;
}

void Avatar::playAttackSound() const{
    static const size_t WEAPON_SLOT = 6;
    const ClientItem *weapon = _gear[WEAPON_SLOT].first;
    const Client &client = *Client::_instance;
    const SoundProfile *weaponSound = weapon == nullptr ? client.avatarSounds() : weapon->sounds();
    if (weaponSound != nullptr)
        weaponSound->playOnce("attack");
}

void Avatar::playDefendSound() const{
    const Client &client = *Client::_instance;
    const ClientItem *armor = getRandomArmor();
    const SoundProfile *armorSound = armor == nullptr ? client.avatarSounds() : armor->sounds();
    if (armorSound != nullptr)
        armorSound->playOnce("defend");
}

void Avatar::onLeftClick(Client &client){
    client.setTarget(*this);
    // Note: parent class's onLeftClick() not called.
}

void Avatar::onRightClick(Client &client){
    client.setTarget(*this, true);
    // Note: parent class's onRightClick() not called.
}

void Avatar::sendTargetMessage() const{
    const Client &client = *Client::_instance;
    client.sendMessage(CL_TARGET_PLAYER, _name);
}

bool Avatar::canBeAttackedByPlayer() const{
    if (! ClientCombatant::canBeAttackedByPlayer())
        return false;
    const Client &client = *Client::_instance;
    return client.isAtWarWith(_name);
}

const Texture &Avatar::cursor(const Client &client) const{
    bool isAtWar = client.isAtWarWith(_name);
    if (isAtWar && isAlive())
        return client.cursorAttack();
    return client.cursorNormal();
}

bool Avatar::belongsToPlayerCity() const{
    bool hasNoCity = _city.empty();
    if (hasNoCity)
        return false;;

    const Avatar &playerCharacter = Client::_instance->character();
    if (_city == playerCharacter._city)
        return true;

    return false;
}

bool Avatar::shouldDrawHealthBar() const{
    const Client &client = *Client::_instance;
    if (this == &client.character())
        return false;
    return ClientCombatant::shouldDrawHealthBar();
}
