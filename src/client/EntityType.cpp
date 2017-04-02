#include <SDL.h>
#include <SDL_image.h>

#include "EntityType.h"
#include "Surface.h"
#include "../Color.h"

EntityType::EntityType(const Rect &drawRect, const std::string &imageFile):
_image(imageFile, Color::MAGENTA),
_drawRect(drawRect),
_isFlat(false),
_isDecoration(false)
{
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}

EntityType::EntityType(Special special):
_isFlat(false)
{
    switch(special){
    case DECORATION:
        _isDecoration = true;
        break;
    }
}

void EntityType::setHighlightImage(const std::string &imageFile){
    Surface highlightSurface(imageFile, Color::MAGENTA);
    if (!highlightSurface)
        return;
    highlightSurface.swapColors(Color::OUTLINE, Color::HIGHLIGHT_OUTLINE);
    _imageHighlight = Texture(highlightSurface);
}

void EntityType::setImage(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}
