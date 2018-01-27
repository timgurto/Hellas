#include <SDL_ttf.h>
#include <sstream>

#include "Texture.h"
#include "WordWrapper.h"

WordWrapper::WordWrapper(TTF_Font *font, px_t width) :
    _width(width) {
    for (char c = '\0'; c != 0x7f; ++c) {
        auto glyph = Texture{ font, std::string{c} };
        _glyphWidths.push_back(glyph.width());
    }
}

WordWrapper::Lines WordWrapper::wrap(const std::string & unwrapped) const {
    //return{ unwrapped };
    auto lines = Lines{};

    std::istringstream iss(unwrapped);
    static const size_t BUFFER_SIZE = 50; // Maximum word length
    static char buffer[BUFFER_SIZE];

    std::string indent;
    while (iss.peek() == ' ') {
        indent += " ";
        iss.ignore();
    }

    std::string segment;
    std::string extraSpaces;
    while (!iss.eof()) {
        iss.get(buffer, BUFFER_SIZE, ' '); iss.ignore(1);
        auto charsRead = iss.gcount();
        std::string word(buffer);
        word = extraSpaces + word;
        extraSpaces = "";
        while (iss.peek() == ' ') {
            extraSpaces += " ";
            iss.ignore(1);
        }
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
        if (segment != "")
            segment += " ";
        segment += word;
    }
    lines.push_back(indent + segment);

    return lines;
}

px_t WordWrapper::getWidth(const std::string & s) const {
    auto total = 0;
    for (char c : s)
        total += _glyphWidths[c];
    return total;
}
