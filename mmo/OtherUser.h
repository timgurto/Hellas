#ifndef OTHER_USER_H
#define OTHER_USER_H

#include <string>

#include "Entity.h"
#include "Point.h"

// A representation of a user other than the one using this client
class OtherUser : public Entity{
    static EntityType _entityType;

    Point _destination;
    std::string _name;

public:
    OtherUser(const std::string &name, const Point &location);

    inline static void image(const std::string &filename) { _entityType.image(filename); }
    inline void destination(const Point &dst) { _destination = dst; }
    inline static const EntityType &entityType() { return _entityType; }

    virtual void draw(const Client &client) const;
    virtual void update(double delta);

    /*
    Get the next location towards destination, with distance determined by
    this client's latency, and by time elapsed.
    This is used to smooth the apparent movement of other users.
    */
    Point interpolatedLocation(double delta);
};

#endif
