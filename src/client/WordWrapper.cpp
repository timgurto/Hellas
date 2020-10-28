#include "WordWrapper.h"

#include <SDL_ttf.h>

#include <sstream>

#include "Texture.h"

WordWrapper::WordWrapper(TTF_Font *font, px_t width) : _width(width) {
  for (char c = '\0'; c != 0x7f; ++c) {
    auto glyph = Texture{font, std::string{c}};
    _glyphWidths.push_back(glyph.width());
  }
  _glyphWidths['\n'] = 0;
}

std::string WordWrapper::readWord(std::istringstream &stream) {
  auto word = std::string{};

  // Gobble leading spaces
  while (stream.peek() == ' ' || stream.peek() == '\n') stream.ignore(1);

  while (true) {
    auto c = static_cast<char>(stream.peek());
    if (!stream) return word;
    if (c == -1) return word;

    if (c == ' ') {
      stream.ignore(1);
      return word;
    }

    if (c == '\n') {
      stream.ignore(1);
      return word + "\n";
    }

    word += c;
    stream.ignore(1);
  }
}

WordWrapper::Lines WordWrapper::wrap(const std::string &unwrapped) const {
  auto lines = Lines{};

  std::istringstream iss(unwrapped);

  bool indentNextWord = false;

  std::string segment;
  while (!iss.eof()) {
    // Handle newlines
    if (!segment.empty() && segment.back() == '\n') {
      segment.pop_back();
      lines.push_back(segment);
      segment = {};
    }

    auto word = readWord(iss);

    // Indent non-first paragraphs
    if (indentNextWord) word = "    " + word;
    indentNextWord = !word.empty() && word.back() == '\n';

    // Add word to line if it fits
    auto lineWidth = getWidth(segment + " " + word);
    if (lineWidth > _width) {
      if (segment == "") {
        lines.push_back("");
        continue;
      } else {
        lines.push_back(segment);
        segment = word;
        continue;
      }
    }
    if (segment != "") segment += " ";
    segment += word;
  }
  lines.push_back(segment);

  return lines;
}

px_t WordWrapper::getWidth(const std::string &s) const {
  auto total = 0;
  for (char c : s) {
    if (c > 127) continue;
    total += _glyphWidths[c];
  }
  return total;
}
