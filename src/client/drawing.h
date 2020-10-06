#pragma once

#include <set>
#include "Sprite.h"

using Sprites = std::set<Sprite *, Sprite::ComparePointers>;

class SpritesToDraw {
 public:
  SpritesToDraw(const Client &client) : _client(client) {}
  void copy(Sprites::const_iterator begin, Sprites::const_iterator end);

  Sprites _container;

 private:
  const Client &_client;
};
