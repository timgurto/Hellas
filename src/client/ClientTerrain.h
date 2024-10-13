#ifndef CLIENT_TERRAIN_H
#define CLIENT_TERRAIN_H

#include <vector>

#include "../Rect.h"
#include "../Terrain.h"
#include "../types.h"
#include "Texture.h"

class ClientTerrain : public Terrain {
  std::vector<Texture> _images;

  // The number of animation frames, or frames from which to choose randomly.
  size_t _frames;

  // If animated, all tiles will show the same frame at any given time.
  size_t _animationFrame;
  ms_t _frameTime;
  ms_t _frameTimer;

  bool _showRandomFrame{false};

  bool _hasHardEdge{false};  // Disable blending with adjacent terrain

 public:
  ClientTerrain(const std::string &imageFile = "", size_t frames = 1,
                ms_t frameTime = 0);

  void setHardEdge() { _hasHardEdge = true; }
  bool hasHardEdge() const { return _hasHardEdge; }

  const Texture &frameToDraw() const;

  void setShowRandomFrame() { _showRandomFrame = true; }

  void draw(const ScreenRect &loc, const ScreenRect &srcRect) const;
  void draw(px_t x, px_t y) const;

  void setFullAlpha() const;
  void setHalfAlpha() const;
  void setQuarterAlpha() const;

  void advanceTime(ms_t timeElapsed);
};

#endif
