#pragma once

#include <set>
#include "Sprite.h"

class SpritesToDraw {
 public:
  SpritesToDraw(const Client &client) : _client(client) {}

  // Set construction
  void add(Sprite::set_t::const_iterator begin,
           Sprite::set_t::const_iterator end);
  void cullAndAdd(const Sprite::set_t &unculledSprites);
  void cullHorizontally(double leftLimit, double rightLimit);

  void drawConstructionSiteFootprints();
  void drawFlatSprites();
  void drawNonFlatSprites();
  void drawDrawOrder();
  void drawCollisionScene();
  void drawNamesAndHealthbars();

 private:
  const Client &_client;
  Sprite::set_t _container;
};
