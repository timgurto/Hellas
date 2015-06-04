#include "Client.h"
#include "OtherUser.h"
#include "util.h"

void OtherUser::updateLocation(double delta){
    if (destination == location)
        return;

    double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    location = interpolate(location, destination, maxLegalDistance);
}