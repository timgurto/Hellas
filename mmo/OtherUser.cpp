#include "Client.h"
#include "OtherUser.h"
#include "util.h"

EntityType OtherUser::_entityType(makeRect(-9, -39));

OtherUser::OtherUser(const std::string &name, const Point &location):
Entity(_entityType, location),
_name(name){}

Point OtherUser::interpolatedLocation(double delta){
    if (_destination == location())
        return _destination;;

    double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(location(), _destination, maxLegalDistance);
}

void OtherUser::draw(const Client &client) const{
    Entity::draw(client);

    // Draw username
    Texture nameTexture(client.defaultFont(), _name, Color::CYAN);
    Point p = location();
    p.y -= 60;
    p.x -= nameTexture.width() / 2;
    nameTexture.draw(p);
}

void OtherUser::update(double delta){
    location(interpolatedLocation(delta));
}
