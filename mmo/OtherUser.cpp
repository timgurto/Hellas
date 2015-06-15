#include "Client.h"
#include "OtherUser.h"
#include "util.h"

EntityType OtherUser::_entityType(makeRect(-9, -39));

OtherUser::OtherUser():
_entity(_entityType, 0){}

Point OtherUser::interpolatedLocation(double delta){
    if (_destination == _entity.location())
        return _destination;;

    double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(_entity.location(), _destination, maxLegalDistance);
}

void OtherUser::setLocation(Entity::set_t &entitiesSet, const Point &newLocation){
    _entity.setLocation(entitiesSet, newLocation);
}
