#include "Client.h"
#include "OtherUser.h"
#include "util.h"

EntityType OtherUser::entityType(makeRect(-9, -39));

OtherUser::OtherUser():
entity(entityType, 0){}

Point OtherUser::interpolatedLocation(double delta){
    if (destination == entity.location())
        return destination;;

    double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(entity.location(), destination, maxLegalDistance);
}