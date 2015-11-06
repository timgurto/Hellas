// (C) 2015 Tim Gurto

#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

#include <string>

#include "Texture.h"
#include "../Point.h"
#include "../util.h"

// Describes a class of Entities, the "instances" of which share common properties
class EntityType{
    Texture _image;
    Rect _drawRect; // Where to draw the image, relative to its location
    bool _isFlat; // Whether these objects appear flat, i.e., are drawn behind all other entities.

public:
    EntityType(const Rect &drawRect = Rect(), const std::string &imageFile = "");
    EntityType(const std::string &id); // For set lookup

    virtual ~EntityType(){}

    void image(const std::string &filename);
    const Texture &image() const { return _image; }
    const Rect &drawRect() const { return _drawRect; }
    void drawRect(const Rect &rect) { _drawRect = rect; }
    bool isFlat() const { return _isFlat; }
    void isFlat(bool b) { _isFlat = b; }
    int width() const { return _drawRect.w; }
    int height() const { return _drawRect.h; }

    void drawAt(const Point &loc) const;
};

#endif
