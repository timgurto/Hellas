#include <SDL.h>
#include <SDL_image.h>

#include "EntityType.h"
#include "Renderer.h"
#include "../Color.h"

extern Renderer renderer;

EntityType::EntityType(const Rect &drawRect, const std::string &imageFile):
_image(imageFile, Color::MAGENTA),
_drawRect(drawRect),
_isFlat(false){
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}

void EntityType::setHighlightImage(const std::string &imageFile){
    SDL_Surface *highlightSurface = IMG_Load(imageFile.c_str());
    if (highlightSurface == nullptr)
        return;
    SDL_SetColorKey(highlightSurface, SDL_TRUE, Color::MAGENTA);

    // Recolor edges
    for (px_t x = 0; x != _drawRect.w; ++x)
        for (px_t y = 0; y != _drawRect.h; ++y){
            Uint32 pixel = renderer.getPixel(highlightSurface, x, y);
            if (pixel == Color::OUTLINE)
                renderer.setPixel(highlightSurface, x, y, Color::HIGHLIGHT_OUTLINE);
        }

    // Convert to Texture
    _imageHighlight = Texture(highlightSurface);
    SDL_FreeSurface(highlightSurface);
}

void EntityType::image(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
    setHighlightImage(imageFile);
}
