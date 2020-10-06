#pragma once

#include <set>
#include "Sprite.h"

using Sprites = std::set<Sprite *, Sprite::ComparePointers>;

class SpritesToDraw {
 public:
  SpritesToDraw(const Client &client) : _client(client) {}

  // Set construction
  void copy(Sprites::const_iterator begin, Sprites::const_iterator end);
  void cullHorizontally(double leftLimit, double rightLimit);

  void drawConstructionSiteFootprints();
  void drawFlatSprites();
  void drawNonFlatSprites();

  Sprites _container;

 private:
  const Client &_client;
};
