#include "ClientTerrain.h"

#include <cassert>
#include <sstream>

#include "../util.h"
#include "Client.h"

ClientTerrain::ClientTerrain(const std::string& imageFile, size_t frames,
                             ms_t frameTime)
    : _frames(frames),
      _animationFrame(0),
      _frameTime(frameTime),
      _frameTimer(frameTime) {
  if (_frames == 1)
    _images.push_back(std::string("Images/Terrain/") + imageFile + ".png");
  else
    for (size_t i = 0; i != frames; ++i) {
      std::ostringstream oss;
      oss << "Images/Terrain/" << imageFile;
      // if (_frames > 100 && i < 100)
      //    oss << "0";
      if (_frames > 10 && i < 10) oss << "0";
      oss << i << ".png";
      _images.push_back(oss.str());
    }

  if (!isDebug())
    for (Texture& frame : _images) {
      frame.setBlend(SDL_BLENDMODE_ADD);
      frame.setAlpha(0x3f);
    }
}

const Texture& ClientTerrain::frameToDraw() const {
  if (_showRandomFrame) {
    auto chosenFrame = rand() % _frames;
    return _images[chosenFrame];
  }
  return _images[_animationFrame];
}

void ClientTerrain::draw(const ScreenRect& loc,
                         const ScreenRect& srcRect) const {
  frameToDraw().draw(loc, srcRect);
}

void ClientTerrain::draw(px_t x, px_t y) const { frameToDraw().draw(x, y); }

void ClientTerrain::setFullAlpha() const { frameToDraw().setAlpha(0xff); }

void ClientTerrain::setHalfAlpha() const { frameToDraw().setAlpha(0x7f); }

void ClientTerrain::setQuarterAlpha() const { frameToDraw().setAlpha(0x3f); }

void ClientTerrain::advanceTime(ms_t timeElapsed) {
  const auto usesAnimation = _frameTime != 0 && !_showRandomFrame;
  if (!usesAnimation) return;

  _frameTimer += timeElapsed;
  if (_frameTimer >= _frameTime * _frames) _frameTimer -= _frameTime * _frames;
  _animationFrame = _frameTimer / _frameTime;
}
