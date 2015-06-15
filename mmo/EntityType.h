#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

#include "Point.h"
#include "Texture.h"
#include "util.h"

// Describes a class of Entities, the "instances" of which share common properties
class EntityType{
    Texture _image;
    SDL_Rect _drawRect; // Where to draw the image, relative to its location

public:
    EntityType(const SDL_Rect &drawRect, const std::string &imageFile = "");

    const SDL_Rect &drawRect() const;
    int width() const;
    int height() const;

    void drawAt(const Point &loc) const;
    void image(const std::string &imageFile);
};

#endif
