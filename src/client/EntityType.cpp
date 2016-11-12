#include <SDL.h>
#include <SDL_image.h>

#include "EntityType.h"
#include "Surface.h"
#include "../Color.h"

EntityType::EntityType(const Rect &drawRect, const std::string &imageFile):
_image(imageFile, Color::MAGENTA),
_drawRect(drawRect),
_isFlat(false){
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}

void EntityType::setHighlightImage(const std::string &imageFile){
    Surface highlightSurface(imageFile, Color::MAGENTA);
    if (!highlightSurface)
        return;

    // Recolor edges
    for (px_t x = 0; x != _drawRect.w; ++x)
        for (px_t y = 0; y != _drawRect.h; ++y){
            if (highlightSurface.getPixel(x, y) == Color::OUTLINE)
                highlightSurface.setPixel(x, y, Color::HIGHLIGHT_OUTLINE);
        }

    // Convert to Texture
    _imageHighlight = Texture(highlightSurface);
}

void EntityType::image(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}
