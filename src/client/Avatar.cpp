// (C) 2015 Tim Gurto

#include "Avatar.h"
#include "Client.h"
#include "Renderer.h"
#include "../util.h"

extern Renderer renderer;

EntityType Avatar::_entityType(Rect(-9, -39));
Rect Avatar::_collisionRect(-5, -2, 10, 4);

Avatar::Avatar(const std::string &name, const Point &location):
Entity(&_entityType, location),
_name(name){}

Point Avatar::interpolatedLocation(double delta){
    if (_destination == location())
        return _destination;;

    const double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(location(), _destination, maxLegalDistance);
}

void Avatar::draw(const Client &client) const{
    Entity::draw(client);

    if (isDebug()) {
        renderer.setDrawColor(Color::GREY_2);
        renderer.drawRect(_collisionRect + location() + client.offset());
    }

    // Draw username
    if (_name != client.username()) {
        const Texture nameTexture(client.defaultFont(), _name, Color::WHITE);
        Point p = location() + client.offset();
        p.y -= 60;
        p.x -= nameTexture.width() / 2;
        nameTexture.draw(p);
    }
}

void Avatar::update(double delta){
    location(interpolatedLocation(delta));
}

std::vector<std::string> Avatar::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back(_name);
    text.push_back("Player");
    return text;
}
