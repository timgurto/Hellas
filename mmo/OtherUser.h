#ifndef OTHER_USER_H
#define OTHER_USER_H

#include "Point.h"

// A representation of a user other than the one using this client
struct  OtherUser{
    Point location;
    Point destination;

    // Set the destination, and reset the travel timer
    void setDestination(const Point &dst);

    // Move location towards destination, with distance determined by
    // this client's latency, and by time elapsed.
    // The idea is that 
    void updateLocation(double delta);

};

#endif