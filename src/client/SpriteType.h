#ifndef SPRITE_TYPE_H
#define SPRITE_TYPE_H

#include <string>

#include "Texture.h"
#include "../Point.h"
#include "../util.h"

// Describes a class of Sprites, the "instances" of which share common properties
class SpriteType{
    Texture _image, _imageHighlight;
    ScreenRect _drawRect; // Where to draw the image, relative to its location
    bool _isFlat; // Whether these objects appear flat, i.e., are drawn behind all other entities.
    bool _isDecoration; // Whether this is invisible to mouse events.

    struct Particles {
        std::string profile;
        MapPoint offset; // From location
    };
    std::vector<Particles> _particles;

public:
    enum Special{
        DECORATION
    };

    SpriteType(const ScreenRect &drawRect = {}, const std::string &imageFile = "");
    SpriteType(Special special);

    virtual ~SpriteType(){}

    void setImage(const std::string &filename);
    const Texture &highlightImage() const { return _imageHighlight; }
    const ScreenRect &drawRect() const { return _drawRect; }
    void drawRect(const ScreenRect &rect) { _drawRect = rect; }
    bool isFlat() const { return _isFlat; }
    void isFlat(bool b) { _isFlat = b; }
    bool isDecoration() const { return _isDecoration; }
    void isDecoration(bool b) { _isDecoration = b; }
    px_t width() const { return _drawRect.w; }
    px_t height() const { return _drawRect.h; }
    void addParticles(const std::string &profileName, const MapPoint &offset);
    const std::vector<Particles> &particles() const { return _particles; }

    void setAlpha(Uint8 alpha) { _image.setAlpha(alpha); }

    void setHighlightImage(const std::string &imageFile);
    virtual const Texture &image() const { return _image; }

    virtual char classTag() const { return 'e'; }
};

#endif
