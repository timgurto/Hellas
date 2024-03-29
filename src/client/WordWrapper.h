#pragma once

#include <SDL_ttf.h>

#include <string>
#include <vector>

#include "../types.h"

class WordWrapper {
 public:
  using Lines = std::vector<std::string>;

  WordWrapper() {}
  WordWrapper(TTF_Font *font, px_t width);

  Lines wrap(const std::string &unwrapped) const;

  static std::string readWord(std::istringstream &stream);

 private:
  px_t _width{0};

  using GlyphWidths = std::vector<px_t>;
  GlyphWidths _glyphWidths;

  px_t getWidth(const std::string &s) const;
};
