#pragma once

#include <set>
#include "Sprite.h"

using Sprites = std::set<Sprite *, Sprite::ComparePointers>;

class SpritesToDraw {
 public:
  static SpritesToDraw Copy(Sprites::const_iterator begin,
                            Sprites::const_iterator end) {
    return {begin, end};
  }
  Sprites _container;

 private:
  SpritesToDraw(Sprites::const_iterator begin, Sprites::const_iterator end);
};
