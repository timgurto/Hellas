#include "Object.h"
#include "Server.h"

void Object::updateLocation(const Point &dest){
    const ms_t newTime = SDL_GetTicks();
    ms_t timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    const double maxLegalDistance = min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES,
                                        timeElapsed + 100) / 1000.0 * speed();
    Point interpolated = interpolate(_location, dest, maxLegalDistance);

    Point newDest = interpolated;
    Server &server = *Server::_instance;

    const User *userPtr = nullptr;
    if (classTag() == 'u')
        userPtr = dynamic_cast<const User *>(this);

    int
        displacementX = toInt(newDest.x - _location.x),
        displacementY = toInt(newDest.y - _location.y);
    Rect journeyRect = collisionRect();
    if (displacementX < 0) {
        journeyRect.x += displacementX;
        journeyRect.w -= displacementX;
    } else {
        journeyRect.w += displacementX;
    }
    if (displacementY < 0) {
        journeyRect.y += displacementY;
        journeyRect.h -= displacementY;
    } else {
        journeyRect.h += displacementY;
    }
    if (!server.isLocationValid(journeyRect, *type(), this)) {
        newDest = _location;
        static const double ACCURACY = 0.5;
        Point testPoint = _location;
        const bool xDeltaPositive = _location.x < interpolated.x;
        do {
            newDest.x = testPoint.x;
            testPoint.x = xDeltaPositive ? (testPoint.x) + ACCURACY : (testPoint.x - ACCURACY);
        } while ((xDeltaPositive ? (testPoint.x <= interpolated.x) :
                                   (testPoint.x >= interpolated.x)) &&
                 server.isLocationValid(testPoint, *type(), this));
        const bool yDeltaPositive = _location.y < interpolated.y;
        testPoint.x = newDest.x; // Keep it valid for y testing.
        do {
            newDest.y = testPoint.y;
            testPoint.y = yDeltaPositive ? (testPoint.y + ACCURACY) : (testPoint.y - ACCURACY);
        } while ((yDeltaPositive ? (testPoint.y <= interpolated.y) :
                                   (testPoint.y >= interpolated.y)) &&
                 server.isLocationValid(testPoint, *type(), this));
    }

    // At this point, the new location has been finalized.  Now new information must be propagated.

    double left, right, top, bottom;
    double forgetLeft, forgetRight, forgetTop, forgetBottom; // Areas newly invisible
    if (newDest.x > _location.x){ // Moved right
        left = _location.x + Server::CULL_DISTANCE;
        right = newDest.x + Server::CULL_DISTANCE;
        forgetLeft = _location.x - Server::CULL_DISTANCE;
        forgetRight = newDest.x - Server::CULL_DISTANCE;
    } else { // Moved left
        left = newDest.x - Server::CULL_DISTANCE;
        right = _location.x - Server::CULL_DISTANCE;
        forgetLeft = newDest.x + Server::CULL_DISTANCE;
        forgetRight = _location.x + Server::CULL_DISTANCE;
    }
    if (newDest.y > _location.y){ // Moved down
        top = _location.y + Server::CULL_DISTANCE;
        bottom = newDest.y + Server::CULL_DISTANCE;
        forgetTop = _location.y - Server::CULL_DISTANCE;
        forgetBottom = newDest.y - Server::CULL_DISTANCE;
    } else { // Moved up
        top = newDest.y - Server::CULL_DISTANCE;
        bottom = _location.y - Server::CULL_DISTANCE;
        forgetTop = newDest.y + Server::CULL_DISTANCE;
        forgetBottom = _location.y + Server::CULL_DISTANCE;
    }

    auto loX = server._objectsByX.lower_bound(&Object(Point(left, 0)));
    auto hiX = server._objectsByX.upper_bound(&Object(Point(right, 0)));
    auto loY = server._objectsByY.lower_bound(&Object(Point(0, top)));
    auto hiY = server._objectsByY.upper_bound(&Object(Point(0, bottom)));
    
    if (classTag() == 'u'){
        server.sendMessage(userPtr->socket(), SV_LOCATION, makeArgs(userPtr->name(),
                                                                    newDest.x, newDest.y));

        // Tell user about any additional objects he can now see
        std::list<const Object *> nearbyObjects;
        for (auto it = loX; it != hiX; ++it){
            if (abs((*it)->location().y - newDest.y) <= Server::CULL_DISTANCE)
                nearbyObjects.push_back(*it);
        }
        for (auto it = loY; it != hiY; ++it){
            double objX = (*it)->location().x;
            if (newDest.x > _location.x){ // Don't count objects twice
                if (objX > left)
                    continue;
            } else {
                if (objX < right)
                    continue;
            }
            if (abs(objX - newDest.x) <= Server::CULL_DISTANCE)
                nearbyObjects.push_back(*it);
        }
        for (const Object *objP : nearbyObjects){
            server.sendObjectInfo(*userPtr, *objP);
        }
    }

    // Tell nearby users that it has moved
    for (const User *userP : server.findUsersInArea(location()))
        if (userP != this)
            if (classTag() == 'u')
                server.sendMessage(userP->socket(), SV_LOCATION, makeArgs(newDest.x, newDest.y));
            else
                server.sendMessage(userP->socket(), SV_OBJECT_LOCATION,
                                   makeArgs(serial(), newDest.x, newDest.y));

    // Tell any users it has moved away from to forget about it.
   auto loUserX = server._usersByX.lower_bound(&User(Point(forgetLeft, 0)));
   auto hiUserX = server._usersByX.upper_bound(&User(Point(forgetRight, 0)));
   auto loUserY = server._usersByY.lower_bound(&User(Point(0, forgetTop)));
   auto hiUserY = server._usersByY.upper_bound(&User(Point(0, forgetBottom)));
   std::list<const User *> usersToForget;
    for (auto it = loUserX; it != hiUserX; ++it){
        double userY = (*it)->location().y;
        if (userY - _location.y <= Server::CULL_DISTANCE)
            usersToForget.push_back(*it);
    }
    for (auto it = loUserY; it != hiUserY; ++it){
        double userX = (*it)->location().x;
        if (newDest.x > _location.x){ // Don't count objects twice.
            if (userX < forgetLeft)
                continue;
        } else {
            if (userX > forgetRight)
                continue;
        }
        if (abs(userX - _location.x) <= Server::CULL_DISTANCE)
            usersToForget.push_back(*it);
    }
    for (const User *userP : usersToForget){
        if (classTag() == 'u')
            server.sendMessage(userP->socket(), SV_USER_OUT_OF_RANGE, userP->name());
        else
            server.sendMessage(userP->socket(), SV_OBJECT_OUT_OF_RANGE, makeArgs(serial()));
    }

    Point oldLoc = _location;

    // Remove from location-indexed trees
    if (classTag() == 'u'){
        if (newDest.x != oldLoc.x)
            server._usersByX.erase(userPtr);
        if (newDest.y != oldLoc.y)
            server._usersByY.erase(userPtr);
    }
    if (newDest.x != oldLoc.x)
        server._objectsByX.erase(this);
    if (newDest.y != oldLoc.y)
        server._objectsByY.erase(this);

    _location = newDest;
    
    // Re-insert into location-indexed trees
    if (classTag() == 'u'){
        if (newDest.x != oldLoc.x)
            server._usersByX.insert(userPtr);
        if (newDest.y != oldLoc.y)
            server._usersByY.insert(userPtr);
    }
    if (newDest.x != oldLoc.x)
        server._objectsByX.insert(this);
    if (newDest.y != oldLoc.y)
        server._objectsByY.insert(this);


}
