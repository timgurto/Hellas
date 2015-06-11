#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

#include "Point.h"
#include "util.h"

// Describes a class of Entities, the "instances" of which share common properties
class EntityType{
    SDL_Surface *_image;
    SDL_Rect _drawRect; // Where to draw the image, relative to its location
    
    static SDL_Surface *_screen;

public:
    EntityType(const SDL_Rect &drawRect, const std::string &imageFile = "");
    ~EntityType();

    void image(const std::string &imageFile);
    const SDL_Rect &drawRect() const;
    int width() const;
    int height() const;

    static void setScreen(SDL_Surface *screen);

    void drawAt(const Point &loc) const;
};

#endif
