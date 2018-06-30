#pragma once

#include <string>
#include "../Point.h"
#include "../types.h"

struct ClientConfig {
  px_t chatW = 150, chatH = 100;

  std::string fontFile = "AdvoCut.ttf";
  int fontSize = 10;
  px_t fontOffset;
  px_t textHeight;

  px_t castBarY = 300, castBarW = 150, castBarH = 11;

  ScreenPoint loginFrontOffset;

  std::string serverHostDirectory;

  void loadFromFile(const std::string &filename);
};
