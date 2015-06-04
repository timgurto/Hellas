#include "Client.h"
#include "OtherUser.h"
#include "util.h"

void OtherUser::updateLocation(double delta){
    if (destination == location)
        return;

    double totalDist = distance(location, destination);
    double distToMove = Client::MOVEMENT_SPEED * delta;
    if (distToMove > totalDist) distToMove = totalDist;
    double
        xNorm = (destination.x - location.x) / totalDist,
        yNorm = (destination.y - location.y) / totalDist;
    location.x += xNorm * distToMove;
    location.y += yNorm * distToMove;
}