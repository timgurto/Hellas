#include <SDL.h>
#include <SDL_image.h>

#include "SpriteType.h"
#include "Surface.h"
#include "../Color.h"

SpriteType::SpriteType(const ScreenRect &drawRect, const std::string &imageFile):
_image(imageFile, Color::MAGENTA),
_drawRect(drawRect),
_isFlat(false),
_isDecoration(false)
{
    if (_image) {
        _drawRect.w = _image.width();
        _drawRect.h = _image.height();
    }
    setHighlightImage(imageFile);
}

SpriteType::SpriteType(Special special):
_isFlat(false)
{
    switch(special){
    case DECORATION:
        _isDecoration = true;
        break;
    }
}

void SpriteType::setHighlightImage(const std::string &imageFile){
    Surface highlightSurface(imageFile, Color::MAGENTA);
    if (!highlightSurface)
        return;
    highlightSurface.swapColors(Color::OUTLINE, Color::HIGHLIGHT_OUTLINE);
    _imageHighlight = Texture(highlightSurface);
}

void SpriteType::setImage(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}
