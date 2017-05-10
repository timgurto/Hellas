#ifndef SPRITE_TYPE_H
#define SPRITE_TYPE_H

#include <string>

#include "Texture.h"
#include "../Point.h"
#include "../util.h"

// Describes a class of Entities, the "instances" of which share common properties
class SpriteType{
    Texture _image, _imageHighlight;
    Rect _drawRect; // Where to draw the image, relative to its location
    bool _isFlat; // Whether these objects appear flat, i.e., are drawn behind all other entities.
    bool _isDecoration; // Whether this is invisible to mouse events.

public:
    enum Special{
        DECORATION
    };

    SpriteType(const Rect &drawRect = Rect(), const std::string &imageFile = "");
    SpriteType(Special special);
    SpriteType(const std::string &id); // For set lookup

    virtual ~SpriteType(){}

    void setImage(const std::string &filename);
    const Texture &highlightImage() const { return _imageHighlight; }
    const Rect &drawRect() const { return _drawRect; }
    void drawRect(const Rect &rect) { _drawRect = rect; }
    bool isFlat() const { return _isFlat; }
    void isFlat(bool b) { _isFlat = b; }
    bool isDecoration() const { return _isDecoration; }
    void isDecoration(bool b) { _isDecoration = b; }
    px_t width() const { return _drawRect.w; }
    px_t height() const { return _drawRect.h; }

    void setHighlightImage(const std::string &imageFile);
    virtual const Texture &image() const { return _image; }

    virtual char classTag() const { return 'e'; }
};

#endif
