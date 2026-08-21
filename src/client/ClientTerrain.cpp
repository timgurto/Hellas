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

const Texture& ClientTerrain::frameToDraw(unsigned coordHash) const {
  if (_showRandomFrame) {
    auto chosenFrame = coordHash % _frames;
    return _images[chosenFrame];
  }
  return _images[_animationFrame];
}

void ClientTerrain::draw(const ScreenRect& loc, const ScreenRect& srcRect,
                         unsigned coordHash) const {
  frameToDraw(coordHash).draw(loc, srcRect);
}

void ClientTerrain::draw(px_t x, px_t y, unsigned coordHash) const {
  frameToDraw(coordHash).draw(x, y);
}

void ClientTerrain::setFullAlpha() const { setTerrainAlpha(0xff); }

void ClientTerrain::setHalfAlpha() const { setTerrainAlpha(0x7f); }

void ClientTerrain::setQuarterAlpha() const { setTerrainAlpha(0x3f); }

void ClientTerrain::setTerrainAlpha(Uint8 alpha) const {
  // Optimisation when not using random frame, since frame selection is
  // predictable
  if (_frameTime) {
    frameToDraw(0).setAlpha(alpha);
    return;
  }

  // Change alpha on all frames
  // TODO try removing this bit and using just the above
  for (auto& frame : _images) {
    frame.setAlpha(alpha);
  }
}

void ClientTerrain::advanceTime(ms_t timeElapsed) {
  const auto usesAnimation = _frameTime != 0;
  if (!usesAnimation) return;

  _frameTimer += timeElapsed;
  if (_frameTimer >= _frameTime * _frames) _frameTimer -= _frameTime * _frames;
  _animationFrame = _frameTimer / _frameTime;
}
