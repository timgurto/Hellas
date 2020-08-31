#ifndef CLIENT_TERRAIN_H
#define CLIENT_TERRAIN_H

#include <vector>

#include "../Rect.h"
#include "../Terrain.h"
#include "../types.h"
#include "Texture.h"

class ClientTerrain : public Terrain {
  std::vector<Texture> _images;
  size_t _frames;
  size_t _frame;
  ms_t _frameTime;
  ms_t _frameTimer;

  bool _hasHardEdge{false};

 public:
  ClientTerrain(const std::string &imageFile = "", size_t frames = 1,
                ms_t frameTime = 0);

  void setHardEdge() { _hasHardEdge = true; }
  bool hasHardEdge() const { return _hasHardEdge; }

  void draw(const ScreenRect &loc, const ScreenRect &srcRect) const;
  void draw(px_t x, px_t y) const;

  void setFullAlpha() const;
  void setHalfAlpha() const;
  void setQuarterAlpha() const;

  void advanceTime(ms_t timeElapsed);
};

#endif
