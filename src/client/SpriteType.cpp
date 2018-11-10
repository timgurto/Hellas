#include <SDL.h>
#include <SDL_image.h>

#include "../Color.h"
#include "Client.h"
#include "SpriteType.h"
#include "Surface.h"

SpriteType::SpriteType(const ScreenRect &drawRect, const std::string &imageFile)
    : _drawRect(drawRect), _isFlat(false), _isDecoration(false) {
  if (imageFile.empty()) return;
  _image = {imageFile, Color::MAGENTA};
  if (_image) {
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
  }
  setHighlightImage(imageFile);
}

SpriteType::SpriteType(Special special) : _isFlat(false) {
  switch (special) {
    case DECORATION:
      _isDecoration = true;
      break;
  }
}

void SpriteType::addParticles(const std::string &profileName,
                              const MapPoint &offset) {
  Particles p;
  p.profile = profileName;
  p.offset = offset;
  _particles.push_back(p);
}

void SpriteType::setHighlightImage(const std::string &imageFile) {
  Surface highlightSurface(imageFile, Color::MAGENTA);
  if (!highlightSurface) return;
  highlightSurface.swapColors(Color::SPRITE_OUTLINE, Color::SPRITE_OUTLINE_HIGHLIGHT);
  _imageHighlight = Texture(highlightSurface);
}

void SpriteType::setImage(const std::string &imageFile) {
  _image = Texture(imageFile, Color::MAGENTA);
  _drawRect.w = _image.width();
  _drawRect.h = _image.height();
  setHighlightImage(imageFile);
}
