// (C) 2015 Tim Gurto

#ifndef AVATAR_H
#define AVATAR_H

#include <string>

#include "Entity.h"
#include "../Point.h"

// The client-side representation of a user, including the player
class Avatar : public Entity{
    static EntityType _entityType;

    Point _destination;
    std::string _name;

public:
    Avatar(const std::string &name = "", const Point &location = 0);

    static void image(const std::string &filename) { _entityType.image(filename); }
    void name(const std::string &newName) { _name = newName; }
    const Point &destination() const { return _destination; }
    void destination(const Point &dst) { _destination = dst; }
    static const EntityType &entityType() { return _entityType; }

    virtual void draw(const Client &client) const override;
    virtual void update(double delta) override;
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const override;

private:
    /*
    Get the next location towards destination, with distance determined by
    this client's latency, and by time elapsed.
    This is used to smooth the apparent movement of other users.
    */
    Point interpolatedLocation(double delta);
};

#endif
