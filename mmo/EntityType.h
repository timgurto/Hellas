// (C) 2015 Tim Gurto

#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

#include <string>

#include "Point.h"
#include "Texture.h"
#include "util.h"

// Describes a class of Entities, the "instances" of which share common properties
class EntityType{
    Texture _image;
    SDL_Rect _drawRect; // Where to draw the image, relative to its location

    // Specifically for object types:
    std::string _id;
    std::string _name;
    bool _canGather; // Whether this represents objects that can be gathered

public:
    EntityType(const SDL_Rect &drawRect = makeRect(), const std::string &imageFile = "",
               const std::string &id = "", const std::string &name = "", bool canGather = false);
    EntityType(const std::string &id); // For set lookup

    const SDL_Rect &drawRect() const { return _drawRect; }
    int width() const { return _drawRect.w; }
    int height() const { return _drawRect.h; }
    bool canGather() const { return _canGather; }
    const std::string &name() const { return _name; }

    // Hacky.  Works, because only types with IDs are used in a set (Client::_objectTypes).
    // TODO: eliminate this, and add custom comparison function for that set.
    bool operator<(const EntityType &rhs) const { return _id < rhs._id; }

    void drawAt(const Point &loc) const;
    void image(const std::string &imageFile);
};

#endif
