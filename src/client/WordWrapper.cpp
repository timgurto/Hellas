#include <SDL_ttf.h>
#include <sstream>

#include "Texture.h"
#include "WordWrapper.h"

WordWrapper::WordWrapper(TTF_Font *font, px_t width) : _width(width) {
  for (char c = '\0'; c != 0x7f; ++c) {
    auto glyph = Texture{font, std::string{c}};
    _glyphWidths.push_back(glyph.width());
  }
  _glyphWidths['\n'] = 100000;  // Force new line
}

std::string WordWrapper::readWord(std::istringstream &stream) {
  auto word = std::string{};
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
  static const size_t BUFFER_SIZE = 50;  // Maximum word length
  static char buffer[BUFFER_SIZE];

  std::string indent;
  while (iss.peek() == ' ') {
    indent += " ";
    iss.ignore();
  }

  std::string segment;
  std::string extraSpaces;
  while (!iss.eof()) {
    // Handle newlines
    auto next = iss.peek();
    if (next == '\n') {
      lines.push_back(indent + segment);
      continue;
    }

    auto word = readWord(iss);

    // Count extra spaces to put before next word
    extraSpaces = "";
    while (iss.peek() == ' ') {
      extraSpaces += " ";
      iss.ignore(1);
    }

    // Add word to line if it fits
    auto lineWidth = getWidth(indent + segment + " " + word);
    if (lineWidth > _width) {
      if (segment == "") {
        lines.push_back("");
        continue;
      } else {
        lines.push_back(indent + segment);
        segment = word;
        continue;
      }
    }
    if (segment != "") segment += " ";
    segment += word;
  }
  lines.push_back(indent + segment);

  return lines;
}

px_t WordWrapper::getWidth(const std::string &s) const {
  auto total = 0;
  for (char c : s) total += _glyphWidths[c];
  return total;
}
