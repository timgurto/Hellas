#ifndef OTHER_USER_H
#define OTHER_USER_H

#include "Entity.h"
#include "Point.h"

// A representation of a user other than the one using this client
class OtherUser{
    Point _destination;
    Entity _entity;
    static EntityType _entityType;

public:
    OtherUser();

    inline static void image(const std::string &filename) { _entityType.image(filename); }
    inline void destination(const Point &dst) { _destination = dst; }
    inline const Entity &entity() const { return _entity; }
    inline static const EntityType &entityType() { return _entityType; }

    /*
    Get the next location towards destination, with distance determined by
    this client's latency, and by time elapsed.
    This is used to smooth the apparent movement of other users.
    */
    Point interpolatedLocation(double delta);

    void setLocation(Entity::set_t &entitiesSet, const Point &newLocation);
};

#endif
