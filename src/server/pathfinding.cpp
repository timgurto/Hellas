#include "Server.h"
#include "objects/Object.h"

Entity::StraightLineMoveResult Entity::moveLegallyTowards(
    const MapPoint &requestedDest,
    ClientLocationUpdateCase whenToSendClientHisLocation) {
  Server &server = *Server::_instance;

  const ms_t newTime = SDL_GetTicks();
  ms_t timeElapsed = newTime - _lastLocUpdate;
  _lastLocUpdate = newTime;

  // Max legal distance: straight line
  double requestedDistance = distance(_location, requestedDest);
  auto distanceToMove = 0.0;
  MapPoint newDest;

  if (shouldMoveWhereverRequested()) {
    distanceToMove = requestedDistance;
    newDest = requestedDest;
  } else {
    distanceToMove = legalMoveDistance(requestedDistance, timeElapsed);
    newDest = interpolate(_location, requestedDest, distanceToMove);

    MapPoint rawDisplacement(newDest.x - _location.x, newDest.y - _location.y);
    auto displacementX = abs(rawDisplacement.x),
         displacementY = abs(rawDisplacement.y);
    auto journeyRect = collisionRect();
    if (rawDisplacement.x < 0) journeyRect.x -= displacementX;
    journeyRect.w += displacementX;
    if (rawDisplacement.y < 0) journeyRect.y -= displacementY;
    journeyRect.h += displacementY;
    if (!server.isLocationValid(journeyRect, *this)) {
      newDest = _location;
      if (!server.isLocationValid(newDest, *this)) {
        SERVER_ERROR(
            "New and previous location are both invalid.  Teleporting "
            "randomly.");
        auto distancesToTryTeleporting = std::vector<int>{10, 20, 30, 50, 100};
        for (auto maxRadius : distancesToTryTeleporting) {
          auto teleportArgs = SpellEffect::Args{};
          teleportArgs.i1 = maxRadius;
          auto randomTeleport = SpellEffect{};
          randomTeleport.args(teleportArgs);
          randomTeleport.setFunction("randomTeleport");
          auto result = randomTeleport.execute(*this, *this);
          if (result == CombatResult::HIT) return TELEPORTED_NEARBY;
        }
        SERVER_ERROR("Failed to find valid place to teleport.");
        return DID_NOT_MOVE;
      }
      static const double ACCURACY = 0.5;
      MapPoint displacementNorm(rawDisplacement.x / distanceToMove * ACCURACY,
                                rawDisplacement.y / distanceToMove * ACCURACY);
      for (double segment = ACCURACY; segment <= distanceToMove;
           segment += ACCURACY) {
        MapPoint testDest = newDest;
        testDest.x += displacementNorm.x;
        if (!server.isLocationValid(testDest, *this)) break;
        newDest = testDest;
      }
      for (double segment = ACCURACY; segment <= distanceToMove;
           segment += ACCURACY) {
        MapPoint testDest = newDest;
        testDest.y += displacementNorm.y;
        if (!server.isLocationValid(testDest, *this)) break;
        newDest = testDest;
      }
    }
  }

  if (newDest == _location) return DID_NOT_MOVE;

  // At this point, the new location has been finalized.  Now new information
  // must be propagated.
  if (!server.isLocationValid(newDest, *this)) {
    SERVER_ERROR("Entity is in invalid location.  Teleporting randomly.");
    auto distancesToTryTeleporting = std::vector<int>{10, 20, 30, 50, 100};
    for (auto maxRadius : distancesToTryTeleporting) {
      auto teleportArgs = SpellEffect::Args{};
      teleportArgs.i1 = maxRadius;
      auto randomTeleport = SpellEffect{};
      randomTeleport.args(teleportArgs);
      randomTeleport.setFunction("randomTeleport");
      auto result = randomTeleport.execute(*this, *this);
      if (result == CombatResult::HIT) return TELEPORTED_NEARBY;
    }
    SERVER_ERROR("Failed to find valid place to teleport.");
    return DID_NOT_MOVE;
  }

  double left, right, top, bottom;  // Area newly visible
  double forgetLeft, forgetRight, forgetTop,
      forgetBottom;               // Area newly invisible
  if (newDest.x > _location.x) {  // Moved right
    left = _location.x + Server::CULL_DISTANCE;
    right = newDest.x + Server::CULL_DISTANCE;
    forgetLeft = _location.x - Server::CULL_DISTANCE;
    forgetRight = newDest.x - Server::CULL_DISTANCE;
  } else {  // Moved left
    left = newDest.x - Server::CULL_DISTANCE;
    right = _location.x - Server::CULL_DISTANCE;
    forgetLeft = newDest.x + Server::CULL_DISTANCE;
    forgetRight = _location.x + Server::CULL_DISTANCE;
  }
  if (newDest.y > _location.y) {  // Moved down
    top = _location.y + Server::CULL_DISTANCE;
    bottom = newDest.y + Server::CULL_DISTANCE;
    forgetTop = _location.y - Server::CULL_DISTANCE;
    forgetBottom = newDest.y - Server::CULL_DISTANCE;
  } else {  // Moved up
    top = newDest.y - Server::CULL_DISTANCE;
    bottom = _location.y - Server::CULL_DISTANCE;
    forgetTop = newDest.y + Server::CULL_DISTANCE;
    forgetBottom = _location.y + Server::CULL_DISTANCE;
  }

  if (classTag() == 'u') {
    auto userPtr = dynamic_cast<const User *>(this);

    auto loX = server._entitiesByX.lower_bound(&Dummy::Location(left, 0));
    auto hiX = server._entitiesByX.upper_bound(&Dummy::Location(right, 0));
    auto loY = server._entitiesByY.lower_bound(&Dummy::Location(0, top));
    auto hiY = server._entitiesByY.upper_bound(&Dummy::Location(0, bottom));

    // Tell user that he has moved
    const auto serverCorrectionWasApplied = newDest != requestedDest;
    const auto shouldUpdateUser =
        (whenToSendClientHisLocation == AlwaysSendUpdate) ||
        serverCorrectionWasApplied;
    if (shouldUpdateUser)
      userPtr->sendMessage(
          {SV_USER_LOCATION, makeArgs(userPtr->name(), newDest.x, newDest.y)});

    // Tell user about any additional objects he can now see
    std::list<const Entity *> nearbyEntities;
    for (auto it = loX; it != hiX; ++it) {
      const Entity &entity = **it;
      char classTag = entity.classTag();
      if (classTag == 'u') continue;

      if (abs(entity.location().y - newDest.y) <= Server::CULL_DISTANCE)
        nearbyEntities.push_back(&entity);
    }
    for (auto it = loY; it != hiY; ++it) {
      const Entity &entity = **it;
      char classTag = entity.classTag();
      if (classTag == 'u') continue;

      double entX = entity.location().x;
      if (newDest.x > _location.x) {  // Don't count objects twice
        if (entX > left) continue;
      } else {
        if (entX < right) continue;
      }
      if (abs(entX - newDest.x) <= Server::CULL_DISTANCE)
        nearbyEntities.push_back(&entity);
    }
    for (const Entity *entP : nearbyEntities) {
      if (!entP->shouldBePropagatedToClients()) continue;
      entP->sendInfoToClient(*userPtr);
    }
  }

  // Assemble list of newly nearby users (used at the end of this function)
  std::list<const User *> newlyNearbyUsers;
  {
    auto loX = server._usersByX.lower_bound(&User(MapPoint{left, 0}));
    auto hiX = server._usersByX.upper_bound(&User(MapPoint{right, 0}));
    auto loY = server._usersByY.lower_bound(&User(MapPoint{0, top}));
    auto hiY = server._usersByY.upper_bound(&User(MapPoint{0, bottom}));
    for (auto it = loX; it != hiX; ++it) {
      if (abs((*it)->location().y - newDest.y) <= Server::CULL_DISTANCE)
        newlyNearbyUsers.push_back(*it);
    }
    for (auto it = loY; it != hiY; ++it) {
      double objX = (*it)->location().x;
      if (newDest.x > _location.x) {  // Don't count users twice
        if (objX > left) continue;
      } else {
        if (objX < right) continue;
      }
      if (abs(objX - newDest.x) <= Server::CULL_DISTANCE)
        newlyNearbyUsers.push_back(*it);
    }
  }

  // Tell nearby users that it has moved
  std::string args;
  if (classTag() == 'u')
    args = makeArgs(dynamic_cast<const User *>(this)->name(), newDest.x,
                    newDest.y);
  else
    args = makeArgs(serial(), newDest.x, newDest.y);
  for (const User *userP : server.findUsersInArea(location())) {
    if (userP == this) continue;
    auto code = classTag() == 'u' ? SV_USER_LOCATION : SV_ENTITY_LOCATION;
    userP->sendMessage({code, args});
  }

  // Tell any users it has moved away from to forget about it, and forget about
  // any such entities.
  std::list<const Entity *> newlyDistantEntities;
  {
    auto loX =
        server._entitiesByX.lower_bound(&Dummy::Location({forgetLeft, 0}));
    auto hiX =
        server._entitiesByX.upper_bound(&Dummy::Location({forgetRight, 0}));
    auto loY =
        server._entitiesByY.lower_bound(&Dummy::Location({0, forgetTop}));
    auto hiY =
        server._entitiesByY.upper_bound(&Dummy::Location({0, forgetBottom}));
    for (auto it = loX; it != hiX; ++it) {
      double entityY = (*it)->location().y;
      if (entityY - _location.y <= Server::CULL_DISTANCE)
        newlyDistantEntities.push_back(*it);
    }
    for (auto it = loY; it != hiY; ++it) {
      double entityX = (*it)->location().x;
      if (newDest.x > _location.x) {  // Don't count users twice.
        if (entityX < forgetLeft) continue;
      } else {
        if (entityX > forgetRight) continue;
      }
      if (abs(entityX - _location.x) <= Server::CULL_DISTANCE)
        newlyDistantEntities.push_back(*it);
    }
  }

  for (const Entity *pEntity : newlyDistantEntities) {
    pEntity->onOutOfRange(*this);
    onOutOfRange(*pEntity);
  }

  // Actually change the entity's location
  location(newDest);

  // Tell newly nearby users that it exists
  for (const User *userP : newlyNearbyUsers) {
    sendInfoToClient(*userP);
    if (classTag() == 'u') {
      User &thisUser = dynamic_cast<User &>(*this);
      userP->sendInfoToClient(thisUser);
    }
  }

  return MOVED;
}
