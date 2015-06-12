#include "Client.h"
#include "OtherUser.h"
#include "util.h"

EntityType OtherUser::_entityType(makeRect(-9, -39));

OtherUser::OtherUser():
_entity(_entityType, 0){}

const EntityType &OtherUser::entityType(){
    return _entityType;
}

void OtherUser::destination(const Point &dst){
    _destination = dst;
}

const Entity &OtherUser::entity() const{
    return _entity;
}

void OtherUser::setImage(const std::string &filename){
    _entityType.image(filename);
}

Point OtherUser::interpolatedLocation(double delta){
    if (_destination == _entity.location())
        return _destination;;

    double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(_entity.location(), _destination, maxLegalDistance);
}

void OtherUser::setLocation(Entity::set_t &entitiesSet, const Point &newLocation){
    _entity.setLocation(entitiesSet, newLocation);
}
