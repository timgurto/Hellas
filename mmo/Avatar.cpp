// (C) 2015 Tim Gurto

#include "Avatar.h"
#include "Client.h"
#include "Server.h"
#include "util.h"

EntityType Avatar::_entityType(makeRect(-9, -31));

Avatar::Avatar(const std::string &name, const Point &location):
Entity(&_entityType, location),
_name(name){}

Point Avatar::interpolatedLocation(double delta){
    if (_destination == location())
        return _destination;;

    const double maxLegalDistance = delta * Server::MOVEMENT_SPEED;
    return interpolate(location(), _destination, maxLegalDistance);
}

void Avatar::draw(const Client &client) const{
    Entity::draw(client);

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
